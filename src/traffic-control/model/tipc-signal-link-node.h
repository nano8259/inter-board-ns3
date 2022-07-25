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
#ifndef TIPC_SIGNAL_LINK_NODE_H
#define TIPC_SIGNAL_LINK_NODE_H

#include "ns3/object.h"
#include "ns3/address.h"
#include "ns3/net-device.h"
#include "ns3/traced-value.h"
#include "ns3/node.h"
#include "ns3/queue-item.h"
#include "traffic-control-layer.h"
#include "tipc-signal-link-monitor.h"
#include "tipc-signal-link.h"
#include "tipc-signal-link-header.h"
#include <map>
#include <vector>

namespace ns3 {

class TipcSignalLink;

/*
 * Link management protocol message types
 */
#define STATE_MSG               0
#define RESET_MSG               1
#define ACTIVATE_MSG            2

#define MAX_BEARERS              3

/* Optional capabilities supported by this code version
 */
enum
{
  TIPC_SYN_BIT          = (1),
  TIPC_BCAST_SYNCH      = (1 << 1),
  TIPC_BCAST_STATE_NACK = (1 << 2),
  TIPC_BLOCK_FLOWCTL    = (1 << 3),
  TIPC_BCAST_RCAST      = (1 << 4),
  TIPC_NODE_ID128       = (1 << 5),
  TIPC_LINK_PROTO_SEQNO = (1 << 6)
};
#define TIPC_NODE_CAPABILITIES (TIPC_SYN_BIT           |  \
                                TIPC_BCAST_SYNCH       |   \
                                TIPC_BCAST_STATE_NACK  |   \
                                TIPC_BCAST_RCAST       |   \
                                TIPC_BLOCK_FLOWCTL     |   \
                                TIPC_NODE_ID128        |   \
                                TIPC_LINK_PROTO_SEQNO)
#define INVALID_BEARER_ID -1


#define INVALID_NODE_SIG        0x10000
#define NODE_CLEANUP_AFTER      300000

/* Flags used to take different actions according to flag type
 * TIPC_NOTIFY_NODE_DOWN: notify node is down
 * TIPC_NOTIFY_NODE_UP: notify node is up
 * TIPC_DISTRIBUTE_NAME: publish or withdraw link state name type
 */
enum
{
  TIPC_NOTIFY_NODE_DOWN           = (1 << 3),
  TIPC_NOTIFY_NODE_UP             = (1 << 4),
  TIPC_NOTIFY_LINK_UP             = (1 << 6),
  TIPC_NOTIFY_LINK_DOWN           = (1 << 7)
};

struct tipc_link_entry
{
  Ptr<TipcSignalLink> link;
  // spinlock_t lock; /* per link */
  uint32_t mtu;
  // struct sk_buff_head inputq;
  // struct tipc_media_addr maddr;
};

/**
 * \defgroup tipc
 *
 * The implementation of TIPC, across multiple layers.
 *
 * \ingroup tipc
 *
 * \brief The TIPC signal link layer implementation.
 *
 * This class is inherent form \see TrafficControlLayer.
 * The TIPC signal link layer logic is implemented in this class.
 * A attribute is used to identify whether the node is host
 * (if not, no need to execute the logics)
 */
class TipcSignalLinkNode : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Get the type ID for the instance
   * \return the instance TypeId
   */
  virtual TypeId GetInstanceTypeId (void) const;

  /**
   * \brief Constructor
   */
  TipcSignalLinkNode ();

  virtual ~TipcSignalLinkNode ();

  // Delete copy constructor and assignment operator to avoid misuse
  TipcSignalLinkNode (TipcSignalLinkNode const &) = delete;
  TipcSignalLinkNode & operator = (TipcSignalLinkNode const &) = delete;

  /* Node FSM states and events:
   */
  enum
  {
    SELF_DOWN_PEER_DOWN    = 0xdd,
    SELF_UP_PEER_UP        = 0xaa,
    SELF_DOWN_PEER_LEAVING = 0xd1,
    SELF_UP_PEER_COMING    = 0xac,
    SELF_COMING_PEER_UP    = 0xca,
    SELF_LEAVING_PEER_DOWN = 0x1d,
    NODE_FAILINGOVER       = 0xf0,
    NODE_SYNCHING          = 0xcc
  };

  enum
  {
    SELF_ESTABL_CONTACT_EVT = 0xece,
    SELF_LOST_CONTACT_EVT   = 0x1ce,
    PEER_ESTABL_CONTACT_EVT = 0x9ece,
    PEER_LOST_CONTACT_EVT   = 0x91ce,
    NODE_FAILOVER_BEGIN_EVT = 0xfbe,
    NODE_FAILOVER_END_EVT   = 0xfee,
    NODE_SYNCH_BEGIN_EVT    = 0xcbe,
    NODE_SYNCH_END_EVT      = 0xcee
  };

  void tipc_node_stop ();
  bool tipc_node_get_id (uint32_t addr, uint8_t *id);
  uint32_t tipc_node_get_addr (struct tipc_node *node);
  uint32_t tipc_node_try_addr (uint8_t *id, uint32_t addr);
  void tipc_node_check_dest (uint32_t onode, uint8_t *peer_id128,
                             struct tipc_bearer *bearer,
                             uint16_t capabilities, uint32_t signature,
                             struct tipc_media_addr *maddr,
                             bool *respond, bool *dupl_addr);
  void tipc_node_delete_links (int bearer_id);
  void tipc_node_apply_property (struct tipc_bearer *b, int prop);
  int tipc_node_get_linkname (uint32_t bearer_id, uint32_t node,
                              char *linkname, size_t len);
  int tipc_node_xmit (struct sk_buff_head *list, uint32_t dnode,
                      int selector);
  int tipc_node_distr_xmit (struct sk_buff_head *list);
  int tipc_node_xmit_skb (struct sk_buff *skb, uint32_t dest,
                          uint32_t selector);
  void tipc_node_subscribe (struct list_head *subscr, uint32_t addr);
  void tipc_node_unsubscribe (struct list_head *subscr, uint32_t addr);
  void tipc_node_broadcast (struct sk_buff *skb);
  int tipc_node_add_conn (uint32_t dnode, uint32_t port, uint32_t peer_port);
  void tipc_node_remove_conn (uint32_t dnode, uint32_t port);
  int tipc_node_get_mtu (uint32_t addr, uint32_t sel);
  bool tipc_node_is_up (uint32_t addr);
  uint16_t tipc_node_get_capabilities (uint32_t addr);
  int tipc_nl_node_dump (struct sk_buff *skb, struct netlink_callback *cb);
  int tipc_nl_node_dump_link (struct sk_buff *skb, struct netlink_callback *cb);
  int tipc_nl_node_reset_link_stats (struct sk_buff *skb, struct genl_info *info);
  int tipc_nl_node_get_link (struct sk_buff *skb, struct genl_info *info);
  int tipc_nl_node_set_link (struct sk_buff *skb, struct genl_info *info);
  int tipc_nl_peer_rm (struct sk_buff *skb, struct genl_info *info);
  int tipc_nl_node_set_monitor (struct sk_buff *skb, struct genl_info *info);
  int tipc_nl_node_get_monitor (struct sk_buff *skb, struct genl_info *info);

protected:

  virtual void DoDispose (void);
  virtual void DoInitialize (void);
  virtual void NotifyNewAggregate (void);

private:
  // Select a active link of the node
  Ptr<TipcSignalLink> node_active_link (int sel);

  // int tipc_node_get_mtu (uint32_t addr, uint32_t sel);

  void tipc_node_write_unlock ();

  void tipc_node_calculate_timer (Ptr<TipcSignalLink>);

  void tipc_node_clear_links ();

  /* tipc_node_cleanup - delete nodes that does not
   * have active links for NODE_CLEANUP_AFTER time
   */
  bool tipc_node_cleanup ();

  /* tipc_node_timeout - handle expiration of node timer
   */
  void tipc_node_timeout();

  /**
   * __tipc_node_link_up - handle addition of link
   * Node lock must be held by caller
   * Link becomes active (alone or shared) or standby, depending on its priority.
   */
  void __tipc_node_link_up(int bearer_id);

  inline bool node_is_up ()
  {
    return m_active_links[0] != INVALID_BEARER_ID;
  }

  /**
   * struct tipc_node - TIPC node structure
   * @addr: network address of node
   * @ref: reference counter to node object
   * @lock: rwlock governing access to structure
   * @net: the applicable net namespace
   * @hash: links to adjacent nodes in unsorted hash chain
   * @inputq: pointer to input queue containing messages for msg event
   * @namedq: pointer to name table input queue with name table messages
   * @active_links: bearer ids of active links, used as index into links[] array
   * @links: array containing references to all links to node
   * @action_flags: bit mask of different types of node actions
   * @state: connectivity state vs peer node
   * @sync_point: sequence number where synch/failover is finished
   * @list: links to adjacent nodes in sorted list of cluster's nodes
   * @working_links: number of working links to node (both active and standby)
   * @link_cnt: number of links to node
   * @capabilities: bitmap, indicating peer node's functional capabilities
   * @signature: node instance identifier
   * @link_id: local and remote bearer ids of changing link, if any
   * @publ_list: list of publications
   * @rcu: rcu struct for tipc_node
   * @delete_at: indicates the time for deleting a down node
   */
  uint32_t m_addr;
// struct kref kref;
// rwlock_t lock;
// struct net *net;
// struct hlist_node hash;  // hash table of nodes
  int m_active_links[2];
  struct tipc_link_entry m_links[MAX_BEARERS];
  // <bearer_id, monitor>
  std::map<uint32_t, Ptr<TipcSignalLinkMonitor> > m_mons;
// struct tipc_bclink_entry bc_entry;  // No bc now
  int m_action_flags;
// struct list_head list;
  int m_state;
  bool m_failover_sent;
  uint16_t m_sync_point;
  int m_link_cnt;
  uint16_t m_working_links;
  uint16_t m_capabilities;
  uint32_t m_signature;
  uint32_t m_link_id;
  std::string m_peer_id;
// struct list_head publ_list;
// struct list_head conn_sks;
  Time m_keepalive_intv;
// struct timer_list timer;

// struct rcu_head rcu;
  Time m_delete_at;

  EventId m_timer{}; //!< timer of the monitor
};


} // namespace ns3

#endif // TIPC_SIGNAL_LINK_NODE_H
