/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Natale Patriciello <natale.patriciello@gmail.com>
 *               2016 Stefano Avallone <stavallo@unina.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "tipc-signal-link-layer.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/log.h"
#include "ns3/object-map.h"
#include "ns3/packet.h"
#include "ns3/socket.h"
#include "ns3/queue-disc.h"
// #include <tuple>
#include <sstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TipcSignalLinkLayer");

NS_OBJECT_ENSURE_REGISTERED (TipcSignalLinkLayer);

// const char* const
// TipcSignalLinkLayer::TipcStateName[8] = { "LINK_ESTABLISHED", "LINK_ESTABLISHING", "LINK_RESET",
//                                           "LINK_RESETTING", "LINK_PEER_RESET", "LINK_FAILINGOVER",
//                                           "LINK_SYNCHING", "LAST_STATE" };

TypeId
TipcSignalLinkLayer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TipcSignalLinkLayer")
    .SetParent<TrafficControlLayer> ()
    .SetGroupName ("Tipc")
    .AddConstructor<TipcSignalLinkLayer> ()
    .AddTraceSource ("TipcState",
                     "Trace TIPC state change of a TIPC signal link layer endpoint",
                     MakeTraceSourceAccessor (&TipcSignalLinkLayer::m_stateTrace),
                     "ns3::TipcSignalLinkLayer::TipcStatesTracedValueCallback")
  ;
  return tid;
}

TypeId
TipcSignalLinkLayer::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

TipcSignalLinkLayer::TipcSignalLinkLayer ()
  : TrafficControlLayer ()
{
  NS_LOG_FUNCTION (this);
}

TipcSignalLinkLayer::~TipcSignalLinkLayer ()
{
  NS_LOG_FUNCTION (this);
}

void
TipcSignalLinkLayer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  // TODO: destruct logic here
  TrafficControlLayer::DoDispose ();
}

void
TipcSignalLinkLayer::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  // TODO: initialize logic here
  TrafficControlLayer::DoInitialize ();
}

void
TipcSignalLinkLayer::NotifyNewAggregate ()
{
  NS_LOG_FUNCTION (this);
  TrafficControlLayer::NotifyNewAggregate ();
}

uint32_t
TipcSignalLinkLayer::LinkFsmEvent (int evt)
{
  NS_LOG_FUNCTION (this << evt);
  uint32_t rc = 0;

  switch (m_state)
    {
      case LINK_RESETTING:
        switch (evt)
          {
            case LINK_PEER_RESET_EVT:
              m_state = LINK_PEER_RESET;
              break;
            case LINK_RESET_EVT:
              m_state = LINK_RESET;
              break;
            case LINK_FAILURE_EVT:
            case LINK_FAILOVER_BEGIN_EVT:
            case LINK_ESTABLISH_EVT:
            case LINK_FAILOVER_END_EVT:
            case LINK_SYNCH_BEGIN_EVT:
            case LINK_SYNCH_END_EVT:
            default:
              goto illegal_evt;
          }
        break;
      case LINK_RESET:
        switch (evt)
          {
            case LINK_PEER_RESET_EVT:
              m_state = LINK_ESTABLISHING;
              break;
            case LINK_FAILOVER_BEGIN_EVT:
              m_state = LINK_FAILINGOVER;
              break;
            case LINK_FAILURE_EVT:
            case LINK_RESET_EVT:
            case LINK_ESTABLISH_EVT:
            case LINK_FAILOVER_END_EVT:
              break;
            case LINK_SYNCH_BEGIN_EVT:
            case LINK_SYNCH_END_EVT:
            default:
              goto illegal_evt;
          }
        break;
      case LINK_PEER_RESET:
        switch (evt)
          {
            case LINK_RESET_EVT:
              m_state = LINK_ESTABLISHING;
              break;
            case LINK_PEER_RESET_EVT:
            case LINK_ESTABLISH_EVT:
            case LINK_FAILURE_EVT:
              break;
            case LINK_SYNCH_BEGIN_EVT:
            case LINK_SYNCH_END_EVT:
            case LINK_FAILOVER_BEGIN_EVT:
            case LINK_FAILOVER_END_EVT:
            default:
              goto illegal_evt;
          }
        break;
      case LINK_FAILINGOVER:
        switch (evt)
          {
            case LINK_FAILOVER_END_EVT:
              m_state = LINK_RESET;
              break;
            case LINK_PEER_RESET_EVT:
            case LINK_RESET_EVT:
            case LINK_ESTABLISH_EVT:
            case LINK_FAILURE_EVT:
              break;
            case LINK_FAILOVER_BEGIN_EVT:
            case LINK_SYNCH_BEGIN_EVT:
            case LINK_SYNCH_END_EVT:
            default:
              goto illegal_evt;
          }
        break;
      case LINK_ESTABLISHING:
        switch (evt)
          {
            case LINK_ESTABLISH_EVT:
              m_state = LINK_ESTABLISHED;
              break;
            case LINK_FAILOVER_BEGIN_EVT:
              m_state = LINK_FAILINGOVER;
              break;
            case LINK_RESET_EVT:
              m_state = LINK_RESET;
              break;
            case LINK_FAILURE_EVT:
            case LINK_PEER_RESET_EVT:
            case LINK_SYNCH_BEGIN_EVT:
            case LINK_FAILOVER_END_EVT:
              break;
            case LINK_SYNCH_END_EVT:
            default:
              goto illegal_evt;
          }
        break;
      case LINK_ESTABLISHED:
        switch (evt)
          {
            case LINK_PEER_RESET_EVT:
              m_state = LINK_PEER_RESET;
              rc |= TIPC_LINK_DOWN_EVT;
              break;
            case LINK_FAILURE_EVT:
              m_state = LINK_RESETTING;
              rc |= TIPC_LINK_DOWN_EVT;
              break;
            case LINK_RESET_EVT:
              m_state = LINK_RESET;
              break;
            case LINK_ESTABLISH_EVT:
            case LINK_SYNCH_END_EVT:
              break;
            case LINK_SYNCH_BEGIN_EVT:
              m_state = LINK_SYNCHING;
              break;
            case LINK_FAILOVER_BEGIN_EVT:
            case LINK_FAILOVER_END_EVT:
            default:
              goto illegal_evt;
          }
        break;
      case LINK_SYNCHING:
        switch (evt)
          {
            case LINK_PEER_RESET_EVT:
              m_state = LINK_PEER_RESET;
              rc |= TIPC_LINK_DOWN_EVT;
              break;
            case LINK_FAILURE_EVT:
              m_state = LINK_RESETTING;
              rc |= TIPC_LINK_DOWN_EVT;
              break;
            case LINK_RESET_EVT:
              m_state = LINK_RESET;
              break;
            case LINK_ESTABLISH_EVT:
            case LINK_SYNCH_BEGIN_EVT:
              break;
            case LINK_SYNCH_END_EVT:
              m_state = LINK_ESTABLISHED;
              break;
            case LINK_FAILOVER_BEGIN_EVT:
            case LINK_FAILOVER_END_EVT:
            default:
              goto illegal_evt;
          }
        break;
      default:
        std::stringstream errSs;
        errSs << "Unknown FSM state " << m_state << "in " << this << std::endl;
        NS_FATAL_ERROR (errSs.str ());
    }
  return rc;
illegal_evt:
  std::stringstream errSs;
  errSs << "Illegal FSM event" << evt << "in state" << m_state << "on link " << this << std::endl;
  NS_FATAL_ERROR (errSs.str ());
  return rc;
}

} // namespace ns3
