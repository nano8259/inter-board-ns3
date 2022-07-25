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
#ifndef TIPC_SIGNAL_LINK_MONITOR_H
#define TIPC_SIGNAL_LINK_MONITOR_H

#include "ns3/object.h"
#include "ns3/address.h"
#include "ns3/net-device.h"
#include "ns3/traced-value.h"
#include "ns3/node.h"
#include "ns3/queue-item.h"
#include <map>
#include <vector>

#define MAX_MON_DOMAIN       64
#define MON_TIMEOUT          120000
#define MAX_PEER_DOWN_EVENTS 4
#define TIPC_DEF_MON_THRESHOLD  32
#define TIPC_CLUSTER_BITS       12
#define TIPC_CLUSTER_SIZE       ((1UL << TIPC_CLUSTER_BITS) - 1)


namespace ns3 {

/**
 * @brief struct tipc_mon_domain: domain record to be transferred between peers
 *
 * @len: actual size of domain record
 * @gen: current generation of sender's domain
 * @ack_gen: most recent generation of self's domain acked by peer
 * @member_cnt: number of domain member nodes described in this record
 * @up_map: bit map indicating which of the members the sender considers up
 * @members: identity of the domain members
 */
struct tipc_mon_domain
{
  uint16_t len;
  uint16_t gen;
  uint16_t ack_gen;
  uint16_t member_cnt;
  uint64_t up_map;
  uint32_t members[MAX_MON_DOMAIN];
};

/**
 * @brief struct tipc_peer: state of a peer node and its domain
 *
 * @addr: tipc node identity of peer
 * @head_map: shows which other nodes currently consider peer 'up'
 * @domain: most recent domain record from peer
 * @hash: position in hashed lookup list
 * @list: position in linked list, in circular ascending order by 'addr'
 * @applied: number of reported domain members applied on this monitor list
 * @is_up: peer is up as seen from this node
 * @is_head: peer is assigned domain head as seen from this node
 * @is_local: peer is in local domain and should be continuously monitored
 * @down_cnt: - numbers of other peers which have reported this on lost
 */
struct tipc_peer
{
  uint32_t addr;
  struct tipc_mon_domain* domain;
  // The hash attribute is the hash of address
  // And the list attribute is the header of the list
  // We use a std::map to make life easier in C++
  // struct hlist_node hash;
  // struct list_head list;
  uint8_t applied;
  uint8_t down_cnt;
  bool is_up;
  bool is_head;
  bool is_local;
};

/**
 * struct tipc_mon_state: link instance's cache of monitor list and domain state
 * @list_gen: current generation of this node's monitor list
 * @gen: current generation of this node's local domain
 * @peer_gen: most recent domain generation received from peer
 * @acked_gen: most recent generation of self's domain acked by peer
 * @monitoring: this peer endpoint should continuously monitored
 * @probing: peer endpoint should be temporarily probed for potential loss
 * @synched: domain record's generation has been synched with peer after reset
 */
struct tipc_mon_state
{
  uint16_t list_gen;
  uint16_t peer_gen;
  uint16_t acked_gen;
  bool monitoring : 1;
  bool probing    : 1;
  bool reset      : 1;
  bool synched    : 1;
};

/**
 * \ingroup tipc
 *
 * \brief The TIPC monitor implementation.
 *
 * This class is used to monitor the TIPC signal link endpoint
 */

class TipcSignalLinkMonitor : public Object
{
public:
  /**
   * Get the type ID.
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \brief TcpSocketState Constructor
   */
  TipcSignalLinkMonitor ();

  ~TipcSignalLinkMonitor (void);

  int tipc_mon_create (int bearer_id);
  void tipc_mon_delete (int bearer_id);
  void tipc_mon_peer_up (uint32_t addr);
  void tipc_mon_peer_down (uint32_t addr, int bearer_id);
  void tipc_mon_prep (void *data, int & dlen,
                      struct tipc_mon_state & state, int bearer_id);
  void tipc_mon_rcv (void *data, uint16_t dlen, uint32_t addr,
                     struct tipc_mon_state & state, int bearer_id);
  void tipc_mon_get_state (uint32_t addr,
                           struct tipc_mon_state & state,
                           int bearer_id);
  void tipc_mon_remove_peer (uint32_t addr, int bearer_id);
  int tipc_nl_monitor_set_threshold (uint32_t cluster_size);
  int tipc_nl_monitor_get_threshold ();
  int __tipc_nl_add_monitor (struct tipc_nl_msg *msg,
                             uint32_t bearer_id);
  int tipc_nl_add_monitor_peer (struct tipc_nl_msg *msg,
                                uint32_t bearer_id, uint32_t *prev_node);
  void mon_timeout();

private:
  // struct hlist_head peers[NODE_HTABLE_SIZE];
  // We use a std::map instead of C style list
  // struct tipc_peer is not a subclass of ns3::Object so we can't use ns3::Ptr
  std::map<uint32_t, struct tipc_peer> m_peers;
  // No need to maintain a separate cnt, we have std::map.size()
  // int m_peerCnt;
  struct tipc_peer m_self;
  // rwlock_t lock;
  struct tipc_mon_domain m_cache;
  uint16_t m_listGen;
  uint16_t m_domGen;
  // TODO: how to replace net?
  // struct net *m_net;
  // TODO: what is timer
  // struct timer_list timer;
  EventId m_timer{}; //!< timer of the monitor
  Time m_timerIntv; // unsigned long in TIPC source code
  uint32_t m_monThreshold;

  tipc_peer & peer_prev (tipc_peer & peer);
  tipc_peer & peer_nxt (tipc_peer & peer);
  tipc_peer & peer_head ();

  uint32_t dom_rec_len (const tipc_mon_domain & dom, uint16_t mcnt);
  uint32_t dom_size (uint32_t peers);
  void map_set (uint64_t & up_map, int i, unsigned int v);
  int map_get (uint64_t & up_map, int i);
  void mon_identify_lost_members (tipc_peer & peer,
                                  tipc_mon_domain & dom_bef,
                                  int applied_bef);
  void mon_apply_domain (tipc_peer & peer);
  void mon_update_local_domain ();
  void mon_update_neighbors (tipc_peer & peer);
  void mon_assign_roles (tipc_peer & head);
  bool tipc_mon_add_peer (uint32_t addr);
  bool tipc_mon_is_active();
  // bool mon_domain_equal(const tipc_mon_domain & d1, const tipc_mon_domain & d2);
};

} // namespace ns3

#endif // TIPC_SIGNAL_LINK_MONITOR_H
