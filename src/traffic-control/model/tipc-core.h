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
#ifndef TIPC_CORE_H
#define TIPC_CORE_H

#include "ns3/object.h"
#include "ns3/address.h"
#include "ns3/net-device.h"
#include "ns3/traced-value.h"
#include "ns3/node.h"
#include "ns3/queue-item.h"
#include "traffic-control-layer.h"
#include "tipc-signal-link-node.h"
#include "tipc-signal-link.h"
#include "tipc-signal-link-monitor.h"
#include <map>
#include <vector>

namespace ns3 {

#define TIPC_MOD_VER "2.0.0"
#define NODE_HTABLE_SIZE       512
#define MAX_BEARERS              3
#define TIPC_DEF_MON_THRESHOLD  32
#define NODE_ID_LEN             16
#define NODE_ID_STR_LEN        (NODE_ID_LEN * 2 + 1)

/* The macros and functions below are deprecated:
 */
#define TIPC_CFG_SRV            0
#define TIPC_ZONE_SCOPE         1
#define TIPC_ADDR_NAMESEQ       1
#define TIPC_ADDR_NAME          2
#define TIPC_ADDR_ID            3
#define TIPC_NODE_BITS          12
#define TIPC_CLUSTER_BITS       12
#define TIPC_ZONE_BITS          8
#define TIPC_NODE_OFFSET        0
#define TIPC_CLUSTER_OFFSET     TIPC_NODE_BITS
#define TIPC_ZONE_OFFSET        (TIPC_CLUSTER_OFFSET + TIPC_CLUSTER_BITS)
#define TIPC_NODE_SIZE          ((1UL << TIPC_NODE_BITS) - 1)
#define TIPC_CLUSTER_SIZE       ((1UL << TIPC_CLUSTER_BITS) - 1)
#define TIPC_ZONE_SIZE          ((1UL << TIPC_ZONE_BITS) - 1)
#define TIPC_NODE_MASK          (TIPC_NODE_SIZE << TIPC_NODE_OFFSET)
#define TIPC_CLUSTER_MASK       (TIPC_CLUSTER_SIZE << TIPC_CLUSTER_OFFSET)
#define TIPC_ZONE_MASK          (TIPC_ZONE_SIZE << TIPC_ZONE_OFFSET)
#define TIPC_ZONE_CLUSTER_MASK (TIPC_ZONE_MASK | TIPC_CLUSTER_MASK)

enum tipc_scope
{
  TIPC_CLUSTER_SCOPE = 2,       /* 0 can also be used */
  TIPC_NODE_SCOPE    = 3
};

/**
 * \defgroup tipc
 *
 * The implementation of TIPC, across multiple layers.
 *
 * \ingroup tipc
 *
 * \brief A class maitain some tipc states and static functions
 */
class TipcCore : public Object
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
  TipcCore ();

  virtual ~TipcCore ();

  // Delete copy constructor and assignment operator to avoid misuse
  TipcCore (TipcCore const &) = delete;
  TipcCore & operator = (TipcCore const &) = delete;

  inline int tipc_netid ()
  {
    return m_net_id;
  }

  inline uint32_t tipc_own_addr ()
  {
    return m_node_addr;
  }

  inline uint8_t* tipc_own_id ()
  {
    return m_node_id;
  }

  static inline uint32_t tipc_cluster_mask (uint32_t addr)
  {
    return addr & TIPC_ZONE_CLUSTER_MASK;
  }
  static inline int tipc_node2scope (uint32_t node)
  {
    return node ? TIPC_NODE_SCOPE : TIPC_CLUSTER_SCOPE;
  }
  inline int tipc_scope2node (int sc)
  {
    return sc != TIPC_NODE_SCOPE ? 0 : tipc_own_addr ();
  }
  inline int in_own_node (uint32_t addr)
  {
    return addr == tipc_own_addr () || !addr;
  }

protected:

  virtual void DoDispose (void);
  virtual void DoInitialize (void);
  virtual void NotifyNewAggregate (void);

private:
  uint8_t  m_node_id[NODE_ID_LEN];
  uint32_t m_node_addr;
  // uint32_t m_trial_addr;
  // unsigned long m_addr_trial_end;
  // We use global addr assign instead of trialing
  static uint32_t global_node_addr;
  char m_node_id_string[NODE_ID_STR_LEN];
  int m_net_id;
  int m_random;
  bool m_legacy_addr_format;

  /* Node table and node list */
  // should be maitain in tipc signal link layer?
  // struct hlist_head node_htable[NODE_HTABLE_SIZE];
  // struct list_head node_list;
  uint32_t m_num_nodes;
  uint32_t m_num_links;

  /* Neighbor monitoring list */
  Ptr<TipcSignalLinkMonitor> m_monitors[MAX_BEARERS];
  int m_mon_threshold;

  /* Bearer list */
  // struct tipc_bearer __rcu *bearer_list[MAX_BEARERS + 1];

  /* Broadcast link */
  // spinlock_t bclock;
  // struct tipc_bc_base *bcbase;
  // struct tipc_link *bcl;

  /* Name table */
  // spinlock_t nametbl_lock;
  // struct name_table *nametbl;

  /* Name dist queue */
  // struct list_head dist_queue;

  /* Topology subscription server */
  // struct tipc_topsrv *topsrv;
  // atomic_t subscription_count;

};


} // namespace ns3

#endif // TIPC_CORE_H
