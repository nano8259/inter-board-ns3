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

#include "tipc-signal-link-node.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/log.h"
#include "ns3/object-map.h"
#include "ns3/packet.h"
#include "ns3/socket.h"
#include "ns3/queue-disc.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
// #include <tuple>
#include <sstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TipcSignalLinkNode");

NS_OBJECT_ENSURE_REGISTERED (TipcSignalLinkNode);

TypeId
TipcSignalLinkNode::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TipcSignalLinkNode")
    .SetParent<TrafficControlLayer> ()
    .SetGroupName ("Tipc")
    .AddConstructor<TipcSignalLinkNode> ()
    .AddAttribute ("Address",
                   "Address of this node",
                   IntegerValue(0), 
                   MakeIntegerAccessor (&TipcSignalLinkNode::m_addr),
                   MakeIntegerChecker<int> (0))
    .AddAttribute ("PeerId",
                   "Peer's id, a string",
                   StringValue(), 
                   MakeStringAccessor (&TipcSignalLinkNode::m_peer_id),
                   nullptr)
    .AddAttribute ("Capabilities",
                   "Capabilities",
                   IntegerValue(0), 
                   MakeIntegerAccessor (&TipcSignalLinkNode::m_capabilities),
                   MakeIntegerChecker<int> (0))
    // .AddTraceSource ("TipcState",
    //                  "Trace TIPC state change of a TIPC signal link layer endpoint",
    //                  MakeTraceSourceAccessor (&TipcSignalLinkNode::m_state),
    //                  "ns3::TracedValueCallback::EcnState")
  ;
  return tid;
}

TypeId
TipcSignalLinkNode::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

TipcSignalLinkNode::TipcSignalLinkNode ()
  : Object ()
{
  NS_LOG_FUNCTION (this);
  // Port from tipc_node_create
  // TODO: The check should perform in the caller
  // n = tipc_node_find(net, addr);
  // if (n) {
  //    if (n->capabilities == capabilities)
  //            goto exit;
  //    /* Same node may come back with new capabilities */
  //    write_lock_bh(&n->lock);
  //    n->capabilities = capabilities;
  //    for (bearer_id = 0; bearer_id < MAX_BEARERS; bearer_id++) {
  //            l = n->links[bearer_id].link;
  //            if (l)
  //                    tipc_link_update_caps(l, capabilities);
  //    }
  //    write_unlock_bh(&n->lock);
  //    goto exit;
  // }

  m_state = SELF_DOWN_PEER_LEAVING;
  m_delete_at = Simulator::Now () + MilliSeconds (NODE_CLEANUP_AFTER);
  m_signature = INVALID_NODE_SIG;
  m_active_links[0] = INVALID_BEARER_ID;
  m_active_links[1] = INVALID_BEARER_ID;

  // No bc now
  // if (!tipc_link_bc_create(net, tipc_own_addr(net),
  //                     addr, U16_MAX,
  //                     tipc_link_window(tipc_bc_sndlink(net)),
  //                     n->capabilities,
  //                     &n->bc_entry.inputq1,
  //                     &n->bc_entry.namedq,
  //                     tipc_bc_sndlink(net),
  //                     &n->bc_entry.link)) {
  //    pr_warn("Broadcast rcv link creation failed, no memory\n");
  //    kfree(n);
  //    n = NULL;
  //    goto exit;
  // }

  // just setup, we do nothing here
  // timer_setup(&n->timer, tipc_node_timeout, 0);
  m_keepalive_intv = MilliSeconds (UINT32_MAX);
}

TipcSignalLinkNode::~TipcSignalLinkNode ()
{
  NS_LOG_FUNCTION (this);
  m_timer.Cancel ();
}

void
TipcSignalLinkNode::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  // TODO: destruct logic here
  Object::DoDispose ();
}

void
TipcSignalLinkNode::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  // TODO: initialize logic here
  Object::DoInitialize ();
}

void
TipcSignalLinkNode::NotifyNewAggregate ()
{
  NS_LOG_FUNCTION (this);
  Object::NotifyNewAggregate ();
}

Ptr<TipcSignalLink>
TipcSignalLinkNode::node_active_link (int sel)
{
  int bearer_id = m_active_links[sel & 1];

  if (bearer_id == INVALID_BEARER_ID)
    {
      return NULL;
    }

  return m_links[bearer_id].link;
}


int
TipcSignalLinkNode::tipc_node_get_mtu (uint32_t addr, uint32_t sel)
{
  int bearer_id;
  unsigned int mtu = MAX_MSG_SIZE;
  // TODO this check should put in the caller
  // n = tipc_node_find(net, addr);
  // if (unlikely(!n))
  //    return mtu;

  bearer_id = m_active_links[sel & 1];
  if (bearer_id != INVALID_BEARER_ID)
    {
      mtu = m_links[bearer_id].mtu;
    }

  return mtu;
}

// TODO: Used only in socket.c, we don't port it now
// bool tipc_node_get_id(struct net *net, uint32_t addr, u8 *id)
// u16 tipc_node_get_capabilities(struct net *net, uint32_t addr)

// TODO: Should implement in caller of node
// static struct tipc_node *tipc_node_find(struct net *net, uint32_t addr)
// static struct tipc_node *tipc_node_find_by_id(struct net *net, u8 *id)

// Locks, but why write_unlock have logic?
// static void tipc_node_read_lock(struct tipc_node *n)
// static void tipc_node_read_unlock(struct tipc_node *n)
// static void tipc_node_write_lock(struct tipc_node *n)
// static void tipc_node_write_unlock_fast(struct tipc_node *n)

void
TipcSignalLinkNode::tipc_node_write_unlock ()
{
  // struct net *net = n->net;
  uint32_t addr = 0;
  uint32_t flags = m_action_flags;
  uint32_t link_id = 0;
  uint32_t bearer_id;

  if (!flags)
    {
      // write_unlock_bh(&n->lock);
      return;
    }
  addr = m_addr;
  link_id = m_link_id;
  bearer_id = link_id & 0xffff;
  // publ_list = &m_publ_list;

  m_action_flags &= ~(TIPC_NOTIFY_NODE_DOWN | TIPC_NOTIFY_NODE_UP |
                      TIPC_NOTIFY_LINK_DOWN | TIPC_NOTIFY_LINK_UP);

  // write_unlock_bh(&n->lock);
  // if (flags & TIPC_NOTIFY_NODE_DOWN)
  //    tipc_publ_notify(net, publ_list, addr);
  // if (flags & TIPC_NOTIFY_NODE_UP)
  //    tipc_named_node_up(net, addr);
  if (flags & TIPC_NOTIFY_LINK_UP)
    {
      m_mons[bearer_id]->tipc_mon_peer_up (addr);
      // tipc_nametbl_publish(net, TIPC_LINK_STATE, addr, addr,
      //                     TIPC_NODE_SCOPE, link_id, link_id);
    }
  if (flags & TIPC_NOTIFY_LINK_DOWN)
    {
      m_mons[bearer_id]->tipc_mon_peer_down (addr, bearer_id);
      // tipc_nametbl_withdraw(net, TIPC_LINK_STATE, addr,
      //                      addr, link_id);
    }
}

void
TipcSignalLinkNode::tipc_node_calculate_timer (Ptr<TipcSignalLink> l)
{
  Time tol = l->tipc_link_tolerance ();
  Time intv = ((tol / 4) > MilliSeconds (500)) ? MilliSeconds (500) : tol / 4;

  /* Link with lowest tolerance determines timer interval */
  if (intv < m_keepalive_intv)
    {
      m_keepalive_intv = intv;
    }

  /* Ensure link's abort limit corresponds to current tolerance */
  l->tipc_link_set_abort_limit (tol.GetMilliSeconds () / m_keepalive_intv.GetMilliSeconds ());
}

// Used in nametable and so on
// void tipc_node_subscribe(struct net *net, struct list_head *subscr, u32 addr)
// void tipc_node_unsubscribe(struct net *net, struct list_head *subscr, u32 addr)


// int tipc_node_add_conn(struct net *net, u32 dnode, u32 port, u32 peer_port)
// void tipc_node_remove_conn(struct net *net, u32 dnode, u32 port)

void
TipcSignalLinkNode::tipc_node_clear_links ()
{
  int i;
  for (i = 0; i < MAX_BEARERS; i++)
    {
      struct tipc_link_entry & le = m_links[i];
      if (le.link)
        {
          le.link = nullptr;
          m_link_cnt--;
        }
    }
}

bool
TipcSignalLinkNode::tipc_node_cleanup ()
{
  bool deleted = false;
  /* If lock held by tipc_node_stop() the node will be deleted anyway */
  // if (!spin_trylock_bh(&tn->node_list_lock))
  //    return false;

  if (!node_is_up () && Simulator::Now () > m_delete_at)
    {
      tipc_node_clear_links ();
      deleted = true;
    }

  return deleted;
}

void
TipcSignalLinkNode::tipc_node_timeout ()
{
  // struct sk_buff_head xmitq;
  int remains = m_link_cnt;
  int bearer_id;
  int rc = 0;

  if (!node_is_up () && tipc_node_cleanup ())
    {
      /*Removing the reference of Timer*/
      // tipc_node_put(n);
      return;
    }

  // __skb_queue_head_init(&xmitq);

  /* Initial node interval to value larger (10 seconds), then it will be
   * recalculated with link lowest tolerance
   */
  m_keepalive_intv = MilliSeconds (10000);
  for (bearer_id = 0; remains && (bearer_id < MAX_BEARERS); bearer_id++)
    {

      struct tipc_link_entry & le = m_links[bearer_id];
      if (le.link)
        {
          /* Link tolerance may change asynchronously: */
          tipc_node_calculate_timer (le.link);
          rc = le.link->tipc_link_timeout ();
          remains--;
        }

      // TODO: How to send a message immediately
      // The xmitq is not hold in this function
      // tipc_bearer_xmit(n->net, bearer_id, &xmitq, &le->maddr);
      if (rc & TipcSignalLink::TIPC_LINK_DOWN_EVT)
        {
          // tipc_node_link_down (bearer_id, false);
        }
    }

  m_timer = Simulator::Schedule (m_keepalive_intv, &TipcSignalLinkNode::tipc_node_timeout, this);
}

// void
// TipcSignalLinkNode::__tipc_node_link_up (int bearer_id)
// {
//   int & slot0 = m_active_links[0];
//   int & slot1 = m_active_links[1];
//   Ptr<TipcSignalLink> ol = node_active_link (0);
//   Ptr<TipcSignalLink> nl = m_links[bearer_id].link;

//   if (!nl || nl->tipc_link_is_up ())
//     {
//       return;
//     }

//   nl->LinkFsmEvent (TipcSignalLink::LINK_ESTABLISH_EVT);
//   if (!tipc_link_is_up (nl))
//     {
//       return;
//     }
//   n->working_links++;
//   n->action_flags |= TIPC_NOTIFY_LINK_UP;
//   n->link_id = tipc_link_id (nl);
//   /* Leave room for tunnel header when returning 'mtu' to users: */
//   n->links[bearer_id].mtu = tipc_link_mtu (nl) - INT_H_SIZE;
//   tipc_bearer_add_dest (n->net, bearer_id, n->addr);
//   tipc_bcast_inc_bearer_dst_cnt (n->net, bearer_id);
//   pr_debug ("Established link <%s> on network plane %c\n",
//             tipc_link_name (nl), tipc_link_plane (nl));
//   trace_tipc_node_link_up (n, true, " ");
//   /* Ensure that a STATE message goes first */
//   tipc_link_build_state_msg (nl, xmitq);
//   /* First link? => give it both slots */
//   if (!ol)
//     {
//       *slot0 = bearer_id;
//       *slot1 = bearer_id;
//       tipc_node_fsm_evt (n, SELF_ESTABL_CONTACT_EVT);
//       n->failover_sent = false;
//       n->action_flags |= TIPC_NOTIFY_NODE_UP;
//       tipc_link_set_active (nl, true);
//       tipc_bcast_add_peer (n->net, nl, xmitq);
//       return;
//     }
//   /* Second link => redistribute slots */
//   if (tipc_link_prio (nl) > tipc_link_prio (ol))
//     {
//       pr_debug ("Old link <%s> becomes standby\n", tipc_link_name (ol));
//       *slot0 = bearer_id;
//       *slot1 = bearer_id;
//       tipc_link_set_active (nl, true);
//       tipc_link_set_active (ol, false);
//     }
//   else if (tipc_link_prio (nl) == tipc_link_prio (ol))
//     {
//       tipc_link_set_active (nl, true);
//       *slot1 = bearer_id;
//     }
//   else
//     {
//       pr_debug ("New link <%s> is standby\n", tipc_link_name (nl));
//     }
//   /* Prepare synchronization with first link */
//   tipc_link_tnl_prepare (ol, nl, SYNCH_MSG, xmitq);
// }

} // namespace ns3
