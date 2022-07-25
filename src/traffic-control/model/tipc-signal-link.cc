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

#include "tipc-signal-link.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/log.h"
#include "ns3/object-map.h"
#include "ns3/packet.h"
#include "ns3/socket.h"
#include "ns3/queue-disc.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
// #include <tuple>
#include <sstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TipcSignalLink");

NS_OBJECT_ENSURE_REGISTERED (TipcSignalLink);

// const char* const
// TipcSignalLink::TipcStateName[8] = { "LINK_ESTABLISHED", "LINK_ESTABLISHING", "LINK_RESET",
//                                           "LINK_RESETTING", "LINK_PEER_RESET", "LINK_FAILINGOVER",
//                                           "LINK_SYNCHING", "LAST_STATE" };

TypeId
TipcSignalLink::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TipcSignalLink")
    .SetParent<TrafficControlLayer> ()
    .SetGroupName ("Tipc")
    .AddConstructor<TipcSignalLink> ()
    .AddAttribute ("TipcCore",
                   "Tipc Core",
                   PointerValue (),
                   MakePointerAccessor (&TipcSignalLink::m_core),
                   MakePointerChecker<TipcCore> ())
    .AddAttribute ("Session",
                   "Session",
                   IntegerValue (0),
                   MakeIntegerAccessor (&TipcSignalLink::m_session),
                   MakeIntegerChecker<uint16_t > (0))
    .AddAttribute ("Peer",
                   "Peer's Address",
                   IntegerValue (0),
                   MakeIntegerAccessor (&TipcSignalLink::m_addr),
                   MakeIntegerChecker<uint32_t > (0))
    .AddAttribute ("Self",
                   "Self",
                   IntegerValue (0),
                   MakeIntegerAccessor (&TipcSignalLink::m_self),
                   MakeIntegerChecker<uint32_t > (0))
    .AddAttribute ("PeerId",
                   "Peer's id, a string",
                   StringValue (),
                   MakeStringAccessor (&TipcSignalLink::m_peer_id),
                   nullptr)
    .AddAttribute ("IfName",
                   "Interface name?",
                   StringValue (),
                   MakeStringAccessor (&TipcSignalLink::m_if_name),
                   nullptr)
    .AddAttribute ("PeerCaps",
                   "Peer's capasity",
                   IntegerValue (0),
                   MakeIntegerAccessor (&TipcSignalLink::m_peer_caps),
                   MakeIntegerChecker<uint16_t > (0))
    .AddAttribute ("InSession",
                   "Is this link in session?",
                   BooleanValue (false),
                   MakeIntegerAccessor (&TipcSignalLink::m_inSession),
                   MakeBooleanChecker ())
    .AddAttribute ("Tolerance",
                   "Tolerance",
                   TimeValue (),
                   MakeTimeAccessor (&TipcSignalLink::m_tolerance),
                   MakeTimeChecker ())
    .AddAttribute ("NetPlane", // the net_plane is a char, there is a cast
                   "NetPlane",
                   IntegerValue (0),
                   MakeIntegerAccessor (&TipcSignalLink::m_net_plane),
                   MakeIntegerChecker<uint8_t> (0))
    .AddAttribute ("AdvertisedMtu",
                   "AdvertisedMtu",
                   IntegerValue (0),
                   MakeIntegerAccessor (&TipcSignalLink::m_advertised_mtu),
                   MakeIntegerChecker<uint16_t> (0))
    .AddAttribute ("Mtu",
                   "Mtu",
                   IntegerValue (0),
                   MakeIntegerAccessor (&TipcSignalLink::m_mtu),
                   MakeIntegerChecker<uint16_t> (0))
    .AddAttribute ("Priority",
                   "Priority",
                   IntegerValue (0),
                   MakeIntegerAccessor (&TipcSignalLink::m_priority),
                   MakeIntegerChecker<uint32_t> (0))
    .AddAttribute ("MinWin",
                   "MinWin",
                   IntegerValue (0),
                   MakeIntegerAccessor (&TipcSignalLink::m_min_win),
                   MakeIntegerChecker<uint32_t> (0))
    .AddAttribute ("MaxWin",
                   "MaxWin",
                   IntegerValue (0),
                   MakeIntegerAccessor (&TipcSignalLink::m_max_win),
                   MakeIntegerChecker<uint32_t> (0))
    .AddTraceSource ("TipcState",
                     "Trace TIPC state change of a TIPC signal link layer endpoint",
                     MakeTraceSourceAccessor (&TipcSignalLink::m_state),
                     "ns3::TracedValueCallback::EcnState")
  ;
  return tid;
}

TypeId
TipcSignalLink::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

TipcSignalLink::TipcSignalLink ()
  : m_addr (0),
    m_session (0),
    m_state (LINK_RESETTING),
    m_inSession (false)
{
  NS_LOG_FUNCTION (this);
  // std::string peer_str;
  std::string self_str;

  m_txBuffer = CreateObject<TipcSignalLinkTxBuffer> ();
  m_rxBuffer = CreateObject<TipcSignalLinkRxBuffer> ();

  // Set link name for unicast links only
  if(!m_peer_id.empty()){
    char* own_id = reinterpret_cast<char*>(m_core->tipc_own_id());
    self_str = std::string(own_id, own_id + strlen(own_id));
    if(self_str.size() > 16){
      self_str = std::to_string(m_self);
    }
    if(m_peer_id.size() > 16){
      m_peer_id = std::to_string(m_addr); // note m_addr is peer's addr
    }
  }
  // Peer i/f name will be completed by reset/activate message
  m_name = self_str + ":";
  m_name += m_if_name + "-";
  m_name += m_peer_id + ":unknown";
  
  tipc_link_set_queue_limits(m_min_win, m_max_win);
}

TipcSignalLink::~TipcSignalLink ()
{
  NS_LOG_FUNCTION (this);
}

void
TipcSignalLink::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  // TODO: destruct logic here
  Object::DoDispose ();
}

void
TipcSignalLink::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  // TODO: initialize logic here
  Object::DoInitialize ();
}

void
TipcSignalLink::NotifyNewAggregate ()
{
  NS_LOG_FUNCTION (this);
  Object::NotifyNewAggregate ();
}

void
TipcSignalLink::Awake ()
{}

void
TipcSignalLink::Reset ()
{
  m_inSession = false;
  m_session++;
}

void
TipcSignalLink::SetMonitor (Ptr<TipcSignalLinkMonitor> monitor)
{
  m_monitor = monitor;
}

void 
TipcSignalLink::tipc_link_set_queue_limits (uint32_t min_win, uint32_t max_win){
  // Used in bc
  // int max_bulk = TIPC_MAX_PUBL / (m_mtu / ITEM_SIZE);
	// l->backlog[TIPC_SYSTEM_IMPORTANCE].limit   = max_bulk;
  
	m_min_win = min_win;
	m_ssthresh = max_win;
	m_max_win = max_win;
	m_window = min_win;
	m_backlog[TIPC_LOW_IMPORTANCE].limit      = min_win * 2;
	m_backlog[TIPC_MEDIUM_IMPORTANCE].limit   = min_win * 4;
	m_backlog[TIPC_HIGH_IMPORTANCE].limit     = min_win * 6;
	m_backlog[TIPC_CRITICAL_IMPORTANCE].limit = min_win * 8;
}

uint32_t
TipcSignalLink::LinkFsmEvent (int evt)
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
// TipcSignalLink::tipc_link_timeout ()
// {
//   uint32_t mtyp = 0;
//   uint32_t rc = 0;
//   bool state = false;
//   bool probe = false;
//   bool setup = false;
//   // uint16_t bc_snt = l->bc_sndlink->snd_nxt - 1;
//   // uint16_t bc_acked = l->bc_rcvlink->acked;
//   // struct tipc_mon_state *mstate = &l->mon_state;
//   // trace_tipc_link_timeout(l, TIPC_DUMP_NONE, " ");
//   // trace_tipc_link_too_silent(l, TIPC_DUMP_ALL, " ");
//   switch (m_state)
//     {
//       case LINK_ESTABLISHED:
//       case LINK_SYNCHING:
//         mtyp = STATE_MSG;
//         // link_profile_stats(l);
//         m_monitor->tipc_mon_get_state (m_addr, m_monState, m_bearer_id);
//         if (m_monState.reset || (m_silent_intv_cnt > m_abort_limit))
//           {
//             return LinkFsmEvent (LINK_FAILURE_EVT);
//           }
//         // state = bc_acked != bc_snt;
//         // state |= l->bc_rcvlink->rcv_unacked;
//         state |= m_rcv_unacked;
//         //  state |= !skb_queue_empty(&l->transmq);
//         //  state |= !skb_queue_empty(&l->deferdq);
//         probe = m_monState.probing;
//         probe |= m_silent_intv_cnt;
//         if (probe || m_monState.monitoring)
//           {
//             m_silent_intv_cnt++;
//           }
//         break;
//       case LINK_RESET:
//         setup = m_rst_cnt++ <= 4;
//         setup |= !(m_rst_cnt % 16);
//         mtyp = RESET_MSG;
//         break;
//       case LINK_ESTABLISHING:
//         setup = true;
//         mtyp = ACTIVATE_MSG;
//         break;
//       case LINK_PEER_RESET:
//       case LINK_RESETTING:
//       case LINK_FAILINGOVER:
//         break;
//       default:
//         break;
//     }
//   if (state || probe || setup)
//     {
//       tipc_link_build_proto_msg (l, mtyp, probe, 0, 0, 0, 0, xmitq);
//     }
//   return rc;
// }

} // namespace ns3
