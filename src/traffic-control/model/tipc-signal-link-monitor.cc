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

#include "tipc-signal-link-monitor.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/log.h"
#include "ns3/fatal-error.h"
#include "ns3/object-map.h"
#include "ns3/packet.h"
#include "ns3/socket.h"
#include "ns3/queue-disc.h"
#include "ns3/simulator.h"
#include "ns3/random-variable-stream.h"
#include <sstream>

#define ntohl(x)        __bswap_32 (x)
#define ntohs(x)        __bswap_16 (x)
#define htonl(x)        __bswap_32 (x)
#define htons(x)        __bswap_16 (x)
#define htonll(x) __bswap_64 (x)

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TipcSignalLinkMonitor");

NS_OBJECT_ENSURE_REGISTERED (TipcSignalLinkMonitor);

TypeId
TipcSignalLinkMonitor::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TipcSignalLinkMonitor")
    .SetParent<Object> ()
    .SetGroupName ("Tipc")
    .AddConstructor <TipcSignalLinkMonitor> ()
    // Seems no attribute should add to the tid
    // .AddAttribute ("PacingCaRatio", "Percent pacing rate increase for congestion avoidance conditions",
    //                UintegerValue (120),
    //                MakeUintegerAccessor (&TcpSocketState::m_pacingCaRatio),
    //                MakeUintegerChecker<uint16_t> ())
    // TODO: no trace yet
    // .AddTraceSource ("RTT",
    //                  "Last RTT sample",
    //                  MakeTraceSourceAccessor (&TcpSocketState::m_lastRtt),
    //                  "ns3::TracedValueCallback::Time")
  ;
  return tid;
}

TipcSignalLinkMonitor::TipcSignalLinkMonitor ()
  : Object ()
{
  struct tipc_mon_domain *dom;

  dom = static_cast<tipc_mon_domain*> (malloc (sizeof(*dom)));
  if (!dom)
    {
      NS_FATAL_ERROR ("No enough space for TIPC Monitor Domain!");
    }

  // No need to init m_peers as a empty map?
  m_self.is_up = true;
  m_self.is_head = true;
  m_self.domain = dom;
  m_monThreshold = TIPC_DEF_MON_THRESHOLD;

  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  m_timerIntv = MilliSeconds (MON_TIMEOUT + x->GetInteger (0, 0xffff));
  m_timer = Simulator::Schedule (m_timerIntv, &TipcSignalLinkMonitor::mon_timeout, this);
}

TipcSignalLinkMonitor::~TipcSignalLinkMonitor ()
{
  for (auto peer_iter = m_peers.begin (); peer_iter != m_peers.end (); peer_iter++)
    {
      free (peer_iter->second.domain);
    }
  free (m_self.domain);
  m_timer.Cancel();
}

// const int tipc_max_domain_size = sizeof(struct tipc_mon_domain);
// const static tipc_mon_domain invalidDomain =
// {
//   0xfffe,
//   0xfffe,
//   0xfffe,
//   0xfffe,
//   0,
//   0
// };

// bool
// TipcSignalLinkMonitor::mon_domain_equal (const tipc_mon_domain & d1, const tipc_mon_domain & d2)
// {
//   return d1.len == d2.len && d1.gen == d2.gen && d1.ack_gen == d2.ack_gen
//          && d1.member_cnt == d2.member_cnt && d1.up_map == d2.up_map
//          && d1.members == d2.members;
// }

/* dom_rec_len(): actual length of domain record for transport
 */
uint32_t
TipcSignalLinkMonitor::dom_rec_len (const tipc_mon_domain & dom, uint16_t mcnt)
{
  // Pointer arithmetic on void* is not allowed by the standard
  return ((char *)&(dom.members) - (char *)&dom) + (mcnt * sizeof(uint32_t));
}

/* dom_size() : calculate size of own domain based on number of peers
 */
uint32_t
TipcSignalLinkMonitor::dom_size (uint32_t peers)
{
  uint32_t i = 0;
  while ((i * i) < peers)
    {
      i++;
    }
  return i < MAX_MON_DOMAIN ? i : MAX_MON_DOMAIN;
}

void
TipcSignalLinkMonitor::map_set (uint64_t & up_map, int i, unsigned int v)
{
  up_map &= ~(1ULL << i);
  up_map |= ((uint64_t)v << i);
}

int
TipcSignalLinkMonitor::map_get (uint64_t & up_map, int i)
{
  return (up_map & (1 << i)) >> i;
}

tipc_peer &
TipcSignalLinkMonitor::peer_prev (tipc_peer &peer)
{
  auto iter_peer = m_peers.find (peer.addr);
  if (iter_peer == m_peers.end ())
    {
      NS_FATAL_ERROR ("The peer is not in the map");
    }
  if (iter_peer == m_peers.begin ()) /*[[unlikely]]*/ {
      // The peer is the first item in the map, return the last one
      return (--m_peers.end ())->second;
    }
  else
    {
      return (--iter_peer)->second;
    }
}

tipc_peer &
TipcSignalLinkMonitor::peer_nxt (tipc_peer &peer)
{
  auto iter_peer = m_peers.find (peer.addr);
  if (iter_peer == m_peers.end ())
    {
      NS_FATAL_ERROR ("The peer is not in the map");
    }
  if (iter_peer == --m_peers.end ()) /*[[unlikely]]*/ {
      // The peer is the last item in the map, return the first one
      return m_peers.begin ()->second;
    }
  else
    {
      return (++iter_peer)->second;
    }
}

// Find the peer with is_head and return it
tipc_peer &
TipcSignalLinkMonitor::peer_head ()
{
  for (auto peer_iter = m_peers.begin (); peer_iter != m_peers.end (); peer_iter++)
    {
      if (peer_iter->second.is_head)
        {
          return peer_iter->second;
        }
    }
  NS_FATAL_ERROR("No head peer!");
  // The following is just a trick to fool the compiler
  tipc_peer * invalidPeer = nullptr;
  return *invalidPeer; 
}
// static struct tipc_peer *get_peer(struct tipc_monitor *mon, u32 addr)
// {
//      struct tipc_peer *peer;
//      unsigned int thash = tipc_hashfn(addr);
//      hlist_for_each_entry(peer, &mon->peers[thash], hash) {
//              if (peer->addr == addr)
//                      return peer;
//      }
//      return NULL;
// }
// static struct tipc_peer *get_self(struct net *net, int bearer_id)
// {
//      struct tipc_monitor *mon = tipc_monitor(net, bearer_id);
//      return mon->self;
// }

bool
TipcSignalLinkMonitor::tipc_mon_is_active ()
{
  //  struct tipc_net *tn = tipc_net(net);
  //  return mon->peer_cnt > tn->mon_threshold;
  return m_peers.size () > m_monThreshold;
}

/* mon_identify_lost_members() : - identify amd mark potentially lost members
 */
void
TipcSignalLinkMonitor::mon_identify_lost_members (struct tipc_peer & peer,
                                                  struct tipc_mon_domain & dom_bef,
                                                  int applied_bef)
{
  struct tipc_peer & member = peer;
  struct tipc_mon_domain & dom_aft = *peer.domain;
  uint8_t  applied_aft = peer.applied;
  uint8_t  i;
  for (i = 0; i < applied_bef; i++)
    {
      member = peer_nxt (member);
      /* Do nothing if self or peer already see member as down */
      if (!member.is_up || !map_get (dom_bef.up_map, i))
        {
          continue;
        }
      /* Loss of local node must be detected by active probing */
      if (member.is_local)
        {
          continue;
        }
      /* Start probing if member was removed from applied domain */
      if (!applied_aft || (applied_aft < i))
        {
          member.down_cnt = 1;
          continue;
        }
      /* Member loss is confirmed if it is still in applied domain */
      if (!map_get (dom_aft.up_map, i))
        {
          member.down_cnt++;
        }
    }
}

/* mon_apply_domain() : match a peer's domain record against monitor list
 */
void
TipcSignalLinkMonitor::mon_apply_domain (tipc_peer & peer)
{
  tipc_mon_domain * dom = peer.domain;
  tipc_peer & member = peer;
  uint32_t addr;
  int i;
  // if (!dom || !peer.is_up)
  // use a static invalidDomain to determine wether the dom is valid
  if (!dom || !peer.is_up)
    {
      return;
    }
  /* Scan across domain members and match against monitor list */
  peer.applied = 0;
  member = peer_nxt (peer);
  for (i = 0; i < dom->member_cnt; i++)
    {
      addr = dom->members[i];
      if (addr != member.addr)
        {
          return;
        }
      peer.applied++;
      member = peer_nxt (member);
    }
}

/* mon_update_local_domain() : update after peer addition/removal/up/down
 */
void
TipcSignalLinkMonitor::mon_update_local_domain ()
{
  struct tipc_peer & self = m_self;
  struct tipc_mon_domain & cache = m_cache;
  struct tipc_mon_domain & dom = *self.domain;
  struct tipc_peer & peer = self;
  uint64_t prev_up_map = dom.up_map;
  uint16_t member_cnt, i;
  bool diff;
  /* Update local domain size based on current size of cluster */
  member_cnt = dom_size (m_peers.size ()) - 1;
  self.applied = member_cnt;
  /* Update native and cached outgoing local domain records */
  dom.len = dom_rec_len (dom, member_cnt);
  diff = dom.member_cnt != member_cnt;
  dom.member_cnt = member_cnt;
  for (i = 0; i < member_cnt; i++)
    {
      peer = peer_nxt (peer);
      diff |= dom.members[i] != peer.addr;
      dom.members[i] = peer.addr;
      map_set (dom.up_map, i, peer.is_up);
      cache.members[i] = htonl (peer.addr);
    }
  diff |= dom.up_map != prev_up_map;
  if (!diff)
    {
      return;
    }
  dom.gen = ++m_domGen;
  cache.len = htons (dom.len);
  cache.gen = htons (dom.gen);
  cache.member_cnt = htons (member_cnt);
  cache.up_map = htonll (dom.up_map);
  mon_apply_domain (self);
}

/* mon_update_neighbors() : update preceding neighbors of added/removed peer
 */
void
TipcSignalLinkMonitor::mon_update_neighbors (tipc_peer & peer)
{
  int dz, i;
  dz = dom_size (m_peers.size ());
  for (i = 0; i < dz; i++)
    {
      mon_apply_domain (peer);
      peer = peer_prev (peer);
    }
}

/* mon_assign_roles() : reassign peer roles after a network change
 * The monitor list is consistent at this stage; i.e., each peer is monitoring
 * a set of domain members as matched between domain record and the monitor list
 */
void
TipcSignalLinkMonitor::mon_assign_roles (tipc_peer & head)
{
  struct tipc_peer & peer = peer_nxt (head);
  struct tipc_peer & self = m_self;
  int i = 0;
  for (; &peer != &self; peer = peer_nxt (peer))
    {
      peer.is_local = false;
      /* Update domain member */
      if (i++ < head.applied)
        {
          peer.is_head = false;
          if (&head == &self)
            {
              peer.is_local = true;
            }
          continue;
        }
      /* Assign next domain head */
      if (!peer.is_up)
        {
          continue;
        }
      if (peer.is_head)
        {
          break;
        }
      head = peer;
      head.is_head = true;
      i = 0;
    }
  m_listGen++;
}

// void tipc_mon_remove_peer (struct net *net, uint32_t addr, int bearer_id)
// {
//   // Find the monitor though net namespace and the bearer_id
//   // We skip this func here because no node or link down now
//   struct tipc_monitor *mon = tipc_monitor (net, bearer_id);
//   struct tipc_peer *self = get_self (net, bearer_id);
//   struct tipc_peer *peer, *prev, *head;
//   write_lock_bh (&mon->lock);
//   peer = get_peer (mon, addr);
//   if (!peer)
//     {
//       goto exit;
//     }
//   prev = peer_prev (peer);
//   list_del (&peer->list);
//   hlist_del (&peer->hash);
//   kfree (peer->domain);
//   kfree (peer);
//   mon->peer_cnt--;
//   head = peer_head (prev);
//   if (head == self)
//     {
//       mon_update_local_domain (mon);
//     }
//   mon_update_neighbors (mon, prev);
//   /* Revert to full-mesh monitoring if we reach threshold */
//   if (!tipc_mon_is_active (net, mon))
//     {
//       list_for_each_entry (peer, &self->list, list) {
//         kfree (peer->domain);
//         peer->domain = NULL;
//         peer->applied = 0;
//       }
//     }
//   mon_assign_roles (mon, head);
// exit:
//   write_unlock_bh (&mon->lock);
// }

bool
TipcSignalLinkMonitor::tipc_mon_add_peer (uint32_t addr)
{
  // The func is rewrite
  tipc_peer peer;
  peer.addr = addr;
  m_peers[addr] = peer;
  mon_update_neighbors (peer);
  return true;
}

void
TipcSignalLinkMonitor::tipc_mon_peer_up (uint32_t addr)
{
  struct tipc_peer & self = m_self;

  if (m_peers.find (addr) == m_peers.end ())
    {
      // This peer is not in the map
      tipc_mon_add_peer (addr);
      return;
    }

  struct tipc_peer & peer = m_peers[addr];
  struct tipc_peer & head = peer_head ();
  peer.is_up = true;
  if (&head == &self)
    {
      mon_update_local_domain ();
    }
  mon_assign_roles (head);
}

void
TipcSignalLinkMonitor::tipc_mon_peer_down (uint32_t addr, int bearer_id)
{
  struct tipc_peer & self = m_self;
  int applied;

  if (m_peers.find (addr) == m_peers.end ())
    {
      NS_LOG_WARN ("Mon: unknown link" << addr << "/" << bearer_id << "DOWN\n");
      return;
    }

  struct tipc_peer & peer = m_peers[addr];
  struct tipc_peer & head = peer_head ();
  struct tipc_mon_domain & dom = *peer.domain;

  applied = peer.applied;
  peer.applied = 0;
  // make the domain unvalid?
  peer.domain = nullptr;
  if (peer.is_head)
    {
      mon_identify_lost_members (peer, dom, applied);
    }
  // kfree (dom);
  peer.is_up = false;
  peer.is_head = false;
  peer.is_local = false;
  peer.down_cnt = 0;

  if (&head == &self)
    {
      mon_update_local_domain ();
    }
  mon_assign_roles (head);
}

/* tipc_mon_rcv - process monitor domain event message
 */
void
TipcSignalLinkMonitor::tipc_mon_rcv (void *data, uint16_t dlen, uint32_t addr,
                                     struct tipc_mon_state & state, int bearer_id)
{
  struct tipc_mon_domain & arrv_dom = *static_cast<tipc_mon_domain*> (data);
  struct tipc_mon_domain dom_bef;
  // struct tipc_mon_domain *dom;
  // struct tipc_peer *peer;
  uint16_t new_member_cnt = ntohs (arrv_dom.member_cnt);
  int new_dlen = dom_rec_len (arrv_dom, new_member_cnt);
  uint16_t new_gen = ntohs (arrv_dom.gen);
  uint16_t acked_gen = ntohs (arrv_dom.ack_gen);
  bool probing = state.probing;
  int i, applied_bef;
  state.probing = false;
  /* Sanity check received domain record */
  if (dlen < dom_rec_len (arrv_dom, 0))
    {
      return;
    }
  if (dlen != dom_rec_len (arrv_dom, new_member_cnt))
    {
      return;
    }
  if ((dlen < new_dlen) || ntohs (arrv_dom.len) != new_dlen)
    {
      return;
    }
  /* Synch generation numbers with peer if link just came up */
  if (!state.synched)
    {
      state.peer_gen = new_gen - 1;
      state.acked_gen = acked_gen;
      state.synched = true;
    }
  if (acked_gen >= state.acked_gen)
    {
      state.acked_gen = acked_gen;
    }
  /* Drop duplicate unless we are waiting for a probe response */
  if (new_gen < state.peer_gen && !probing)
    {
      return;
    }

  // write_lock_bh (&mon->lock);
  if (m_peers.find (addr) == m_peers.end ())
    {
      return;
    }
  struct tipc_peer & peer = m_peers[addr];
  if (!peer.is_up)
    {
      return;
    }
  /* Peer is confirmed, stop any ongoing probing */
  peer.down_cnt = 0;
  /* Task is done for duplicate record */
  if (new_gen < state.peer_gen)
    {
      return;
    }
  state.peer_gen = new_gen;
  /* Cache current domain record for later use */
  dom_bef.member_cnt = 0;
  struct tipc_mon_domain * dom = peer.domain;
  if (dom)
    {
      memcpy (&dom_bef, dom, dom->len);
    }
  /* Transform and store received domain record */
  if (!dom || (dom->len < new_dlen))
    {
      // kfree (dom);
      free (dom);
      dom = static_cast<tipc_mon_domain*> (malloc (new_dlen));
      // dom = kmalloc (new_dlen, GFP_ATOMIC);
      peer.domain = dom;
      if (!dom)
        {
          NS_FATAL_ERROR ("No enough space");
          return;
        }
    }
  dom->len = new_dlen;
  dom->gen = new_gen;
  dom->member_cnt = new_member_cnt;
  dom->up_map = htonll (arrv_dom.up_map);
  for (i = 0; i < new_member_cnt; i++)
    {
      dom->members[i] = ntohl (arrv_dom.members[i]);
    }
  /* Update peers affected by this domain record */
  applied_bef = peer.applied;
  mon_apply_domain (peer);
  mon_identify_lost_members (peer, dom_bef, applied_bef);
  mon_assign_roles (peer_head ());
// exit:
// write_unlock_bh (&mon->lock);
}

void
TipcSignalLinkMonitor::tipc_mon_prep (void *data, int & dlen,
                                      struct tipc_mon_state & state, int bearer_id)
{
  struct tipc_mon_domain & dom = *static_cast<tipc_mon_domain*> (data);
  uint16_t gen = m_domGen;
  uint16_t len;

  /* Send invalid record if not active */
  if (!tipc_mon_is_active ())
    {
      dom.len = 0;
      return;
    }

  /* Send only a dummy record with ack if peer has acked our last sent */
  if ((state.acked_gen == gen))
    {
      len = dom_rec_len (dom, 0);
      dlen = len;
      dom.len = htons (len);
      dom.gen = htons (gen);
      dom.ack_gen = htons (state.peer_gen);
      dom.member_cnt = 0;
      return;
    }

  /* Send the full record */
  // read_lock_bh (&mon->lock);
  len = ntohs (m_cache.len);
  dlen = len;
  memcpy (data, &m_cache, len);
  // read_unlock_bh (&mon->lock);
  dom.ack_gen = htons (state.peer_gen);
}

void
TipcSignalLinkMonitor::tipc_mon_get_state (uint32_t addr,
                                           struct tipc_mon_state & state,
                                           int bearer_id)
{
  if (!tipc_mon_is_active ())
    {
      state.probing = false;
      state.monitoring = true;
      return;
    }

  /* Used cached state if table has not changed */
  if (!state.probing
      && (state.list_gen == m_listGen)
      && (state.acked_gen == m_domGen))
    {
      return;
    }
  // read_lock_bh (&mon->lock);
  if (m_peers.find (addr) == m_peers.end ())
    {
      NS_FATAL_ERROR ("No such peer");
      return;
    }
  struct tipc_peer & peer = m_peers[addr];

  state.probing = state.acked_gen != m_domGen;
  state.probing |= peer.down_cnt;
  state.reset |= peer.down_cnt >= MAX_PEER_DOWN_EVENTS;
  state.monitoring = peer.is_local;
  state.monitoring |= peer.is_head;
  state.list_gen = m_listGen;

  // read_unlock_bh (&mon->lock);
}

void
TipcSignalLinkMonitor::mon_timeout ()
{
  // struct tipc_monitor *mon = from_timer (mon, t, timer);
  struct tipc_peer & self = m_self;
  int best_member_cnt = dom_size (m_peers.size ()) - 1;
  // write_lock_bh (&mon->lock);
  if (best_member_cnt != self.applied)
    {
      mon_update_local_domain ();
      mon_assign_roles (self);
    }
  // write_unlock_bh (&mon->lock);
  // mod_timer (&mon->timer, jiffies + mon->timer_intv);
  m_timer = Simulator::Schedule (m_timerIntv, &TipcSignalLinkMonitor::mon_timeout, this);
}


int 
TipcSignalLinkMonitor::tipc_nl_monitor_set_threshold (uint32_t cluster_size)
{
  if (cluster_size > TIPC_CLUSTER_SIZE)
    {
      return -EINVAL;
    }
  m_monThreshold = cluster_size;
  return 0;
}

int 
TipcSignalLinkMonitor::tipc_nl_monitor_get_threshold ()
{
  return m_monThreshold;
}

// int 
// TipcSignalLinkMonitor::__tipc_nl_add_monitor_peer (struct tipc_peer *peer,
//                                 struct tipc_nl_msg *msg)
// {
//   struct tipc_mon_domain *dom = peer->domain;
//   struct nlattr *attrs;
//   void *hdr;
//   hdr = genlmsg_put (msg->skb, msg->portid, msg->seq, &tipc_genl_family,
//                      NLM_F_MULTI, TIPC_NL_MON_PEER_GET);
//   if (!hdr)
//     {
//       return -EMSGSIZE;
//     }
//   attrs = nla_nest_start (msg->skb, TIPC_NLA_MON_PEER);
//   if (!attrs)
//     {
//       goto msg_full;
//     }
//   if (nla_put_u32 (msg->skb, TIPC_NLA_MON_PEER_ADDR, peer->addr))
//     {
//       goto attr_msg_full;
//     }
//   if (nla_put_u32 (msg->skb, TIPC_NLA_MON_PEER_APPLIED, peer->applied))
//     {
//       goto attr_msg_full;
//     }
//   if (peer->is_up)
//     {
//       if (nla_put_flag (msg->skb, TIPC_NLA_MON_PEER_UP))
//         {
//           goto attr_msg_full;
//         }
//     }
//   if (peer->is_local)
//     {
//       if (nla_put_flag (msg->skb, TIPC_NLA_MON_PEER_LOCAL))
//         {
//           goto attr_msg_full;
//         }
//     }
//   if (peer->is_head)
//     {
//       if (nla_put_flag (msg->skb, TIPC_NLA_MON_PEER_HEAD))
//         {
//           goto attr_msg_full;
//         }
//     }
//   if (dom)
//     {
//       if (nla_put_u32 (msg->skb, TIPC_NLA_MON_PEER_DOMGEN, dom->gen))
//         {
//           goto attr_msg_full;
//         }
//       if (nla_put_u64_64bit (msg->skb, TIPC_NLA_MON_PEER_UPMAP,
//                              dom->up_map, TIPC_NLA_MON_PEER_PAD))
//         {
//           goto attr_msg_full;
//         }
//       if (nla_put (msg->skb, TIPC_NLA_MON_PEER_MEMBERS,
//                    dom->member_cnt * sizeof(u32), &dom->members))
//         {
//           goto attr_msg_full;
//         }
//     }
//   nla_nest_end (msg->skb, attrs);
//   genlmsg_end (msg->skb, hdr);
//   return 0;
// attr_msg_full:
//   nla_nest_cancel (msg->skb, attrs);
// msg_full:
//   genlmsg_cancel (msg->skb, hdr);
//   return -EMSGSIZE;
// }
// int tipc_nl_add_monitor_peer (struct net *net, struct tipc_nl_msg *msg,
//                               u32 bearer_id, u32 *prev_node)
// {
//   struct tipc_monitor *mon = tipc_monitor (net, bearer_id);
//   struct tipc_peer *peer;
//   if (!mon)
//     {
//       return -EINVAL;
//     }
//   read_lock_bh (&mon->lock);
//   peer = mon->self;
//   do
//     {
//       if (*prev_node)
//         {
//           if (peer->addr == *prev_node)
//             {
//               *prev_node = 0;
//             }
//           else
//             {
//               continue;
//             }
//         }
//       if (__tipc_nl_add_monitor_peer (peer, msg))
//         {
//           *prev_node = peer->addr;
//           read_unlock_bh (&mon->lock);
//           return -EMSGSIZE;
//         }
//     }
//   while ((peer = peer_nxt (peer)) != mon->self);
//   read_unlock_bh (&mon->lock);
//   return 0;
// }
// int __tipc_nl_add_monitor (struct net *net, struct tipc_nl_msg *msg,
//                            u32 bearer_id)
// {
//   struct tipc_monitor *mon = tipc_monitor (net, bearer_id);
//   char bearer_name[TIPC_MAX_BEARER_NAME];
//   struct nlattr *attrs;
//   void *hdr;
//   int ret;
//   ret = tipc_bearer_get_name (net, bearer_name, bearer_id);
//   if (ret || !mon)
//     {
//       return 0;
//     }
//   hdr = genlmsg_put (msg->skb, msg->portid, msg->seq, &tipc_genl_family,
//                      NLM_F_MULTI, TIPC_NL_MON_GET);
//   if (!hdr)
//     {
//       return -EMSGSIZE;
//     }
//   attrs = nla_nest_start (msg->skb, TIPC_NLA_MON);
//   if (!attrs)
//     {
//       goto msg_full;
//     }
//   read_lock_bh (&mon->lock);
//   if (nla_put_u32 (msg->skb, TIPC_NLA_MON_REF, bearer_id))
//     {
//       goto attr_msg_full;
//     }
//   if (tipc_mon_is_active (net, mon))
//     {
//       if (nla_put_flag (msg->skb, TIPC_NLA_MON_ACTIVE))
//         {
//           goto attr_msg_full;
//         }
//     }
//   if (nla_put_string (msg->skb, TIPC_NLA_MON_BEARER_NAME, bearer_name))
//     {
//       goto attr_msg_full;
//     }
//   if (nla_put_u32 (msg->skb, TIPC_NLA_MON_PEERCNT, mon->peer_cnt))
//     {
//       goto attr_msg_full;
//     }
//   if (nla_put_u32 (msg->skb, TIPC_NLA_MON_LISTGEN, mon->list_gen))
//     {
//       goto attr_msg_full;
//     }
//   read_unlock_bh (&mon->lock);
//   nla_nest_end (msg->skb, attrs);
//   genlmsg_end (msg->skb, hdr);
//   return 0;
// attr_msg_full:
//   read_unlock_bh (&mon->lock);
//   nla_nest_cancel (msg->skb, attrs);
// msg_full:
//   genlmsg_cancel (msg->skb, hdr);
//   return -EMSGSIZE;
// }


} // namespace ns3
