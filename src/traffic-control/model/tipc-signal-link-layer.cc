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
                     MakeTraceSourceAccessor (&TipcSignalLinkLayer::m_state),
                     "ns3::TracedValueCallback::EcnState")
  ;
  return tid;
}

TypeId
TipcSignalLinkLayer::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

TipcSignalLinkLayer::TipcSignalLinkLayer ()
  : TrafficControlLayer (),
    m_peerAddr (0),
    m_session (0),
    m_state (LINK_RESETTING),
    m_inSession (false)
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

void
TipcSignalLinkLayer::Awake ()
{}

void
TipcSignalLinkLayer::Reset ()
{
  m_inSession = false;
  m_session++;
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

// uint32_t
// TipcSignalLinkLayer::tipc_link_timeout()
// {
//      uint32_t mtyp = 0;
//      uint32_t rc = 0;
//      bool state = false;
//      bool probe = false;
//      bool setup = false;
//      // uint16_t bc_snt = l->bc_sndlink->snd_nxt - 1;
//      // uint16_t bc_acked = l->bc_rcvlink->acked;
//      // struct tipc_mon_state *mstate = &l->mon_state;
//      // trace_tipc_link_timeout(l, TIPC_DUMP_NONE, " ");
//      // trace_tipc_link_too_silent(l, TIPC_DUMP_ALL, " ");
//      switch (m_state) {
//      case LINK_ESTABLISHED:
//      case LINK_SYNCHING:
//              mtyp = STATE_MSG;
//              // link_profile_stats(l);
//              // tipc_mon_get_state(l->net, l->addr, mstate, l->bearer_id);
//              // if (mstate->reset || (l->silent_intv_cnt > l->abort_limit))
//              //      return tipc_link_fsm_evt(l, LINK_FAILURE_EVT);
//              // state = bc_acked != bc_snt;
//              // state |= l->bc_rcvlink->rcv_unacked;
//              // state |= l->rcv_unacked;
//              state |= !skb_queue_empty(&l->transmq);
//              state |= !skb_queue_empty(&l->deferdq);
//              probe = mstate->probing;
//              probe |= l->silent_intv_cnt;
//              if (probe || mstate->monitoring)
//                      l->silent_intv_cnt++;
//              break;
//      case LINK_RESET:
//              setup = l->rst_cnt++ <= 4;
//              setup |= !(l->rst_cnt % 16);
//              mtyp = RESET_MSG;
//              break;
//      case LINK_ESTABLISHING:
//              setup = true;
//              mtyp = ACTIVATE_MSG;
//              break;
//      case LINK_PEER_RESET:
//      case LINK_RESETTING:
//      case LINK_FAILINGOVER:
//              break;
//      default:
//              break;
//      }
//      if (state || probe || setup)
//              tipc_link_build_proto_msg(l, mtyp, probe, 0, 0, 0, 0, xmitq);
//      return rc;
// }

} // namespace ns3
