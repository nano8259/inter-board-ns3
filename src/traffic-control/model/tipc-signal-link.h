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
#ifndef TIPC_SIGNAL_LINK_H
#define TIPC_SIGNAL_LINK_H

#include "ns3/object.h"
#include "ns3/address.h"
#include "ns3/net-device.h"
#include "ns3/traced-value.h"
#include "ns3/node.h"
#include "ns3/queue-item.h"
#include "traffic-control-layer.h"
#include "tipc-signal-link-monitor.h"
#include "tipc-signal-link-tx-buffer.h"
#include "tipc-signal-link-rx-buffer.h"
#include "ns3/tipc-core.h"
#include <map>
#include <vector>

namespace ns3 {

class Packet;
class QueueDisc;
class NetDeviceQueueInterface;

/*
 * Link management protocol message types
 */
#define STATE_MSG               0
#define RESET_MSG               1
#define ACTIVATE_MSG            2

#define TIPC_MAX_SUBSCR         65535
#define TIPC_MAX_PUBL           65535


/*
 * Message importance levels
 */
#define TIPC_LOW_IMPORTANCE             0
#define TIPC_MEDIUM_IMPORTANCE          1
#define TIPC_HIGH_IMPORTANCE            2
#define TIPC_CRITICAL_IMPORTANCE        3

class TipcCore;

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
class TipcSignalLink : public Object
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
  TipcSignalLink ();

  virtual ~TipcSignalLink ();

  // Delete copy constructor and assignment operator to avoid misuse
  TipcSignalLink (TipcSignalLink const &) = delete;
  TipcSignalLink & operator = (TipcSignalLink const &) = delete;

  /**
   * \ingroup tipc
   * \brief Names of the 7 TIPC states
   *
   */
  typedef enum
  {
    LINK_ESTABLISHED     = 0xe,       /**< Working-Working */
    LINK_ESTABLISHING    = 0xe  << 4,
    LINK_RESET           = 0x1  << 8,
    LINK_RESETTING       = 0x2  << 12,
    LINK_PEER_RESET      = 0xd  << 16,
    LINK_FAILINGOVER     = 0xf  << 20,
    LINK_SYNCHING        = 0xc  << 24,
    LAST_STATE                        /**< Last state, used only in debug messages                */
  } TipcStates_t;

  /**
   * \ingroup tipc
   * \brief Names of the 8 TIPC events
   *
   */
  typedef enum
  {
    LINK_ESTABLISH_EVT       = 0xec1ab1e,
    LINK_PEER_RESET_EVT      = 0x9eed0e,
    LINK_FAILURE_EVT         = 0xfa110e,
    LINK_RESET_EVT           = 0x10ca1d0e,
    LINK_FAILOVER_BEGIN_EVT  = 0xfa110bee,
    LINK_FAILOVER_END_EVT    = 0xfa110ede,
    LINK_SYNCH_BEGIN_EVT     = 0xc1ccbee,
    LINK_SYNCH_END_EVT       = 0xc1ccede
  } TipcEvents_t;

  /**
   * \ingroup tipc
   * \brief Names of the 3 TIPC link events
   *
   */
  typedef enum
  {
    TIPC_LINK_UP_EVT       = 1,
    TIPC_LINK_DOWN_EVT     = (1 << 1),
    TIPC_LINK_SND_STATE    = (1 << 2)
  } TipcLinkEvents_t;

  /**
   * \brief Awake the TIPC signal layer endpoint.
   *
   * This function should be called at the creation, only once.
   */
  void Awake ();

  /**
   * \brief Reset the TIPC signal layer endpoint.
   *
   * Called at awake or reset state.
   */
  void Reset ();

  void SetMonitor (Ptr<TipcSignalLinkMonitor> monitor);

  uint32_t tipc_link_timeout ();

  /**
   * \brief Link finite state machine
   *
   * \param event state machine event to be processed
   */
  uint32_t LinkFsmEvent (int evt);

  inline Time tipc_link_tolerance ()
  {
    return m_tolerance;
  }

  inline void tipc_link_set_abort_limit (uint32_t limit)
  {
    m_abort_limit = limit;
  }

  inline int tipc_link_is_up ()
  {
    return m_state & (LINK_ESTABLISHED | LINK_SYNCHING);
  }

  inline bool tipc_link_peer_is_down ()
  {
    return m_state == LINK_PEER_RESET;
  }
  inline bool tipc_link_is_reset ()
  {
    return m_state & (LINK_RESET | LINK_FAILINGOVER | LINK_ESTABLISHING);
  }
  inline bool tipc_link_is_establishing ()
  {
    return m_state == LINK_ESTABLISHING;
  }
  inline bool tipc_link_is_synching ()
  {
    return m_state == LINK_SYNCHING;
  }
  inline bool tipc_link_is_failingover ()
  {
    return m_state == LINK_FAILINGOVER;
  }
  inline bool tipc_link_is_blocked ()
  {
    return m_state & (LINK_RESETTING | LINK_PEER_RESET | LINK_FAILINGOVER);
  }
  inline void tipc_link_set_active (bool active)
  {
    m_active = active;
  }
  inline uint32_t tipc_link_id ()
  {
    return m_peer_bearer_id << 16 | m_bearer_id;
  }
  inline int tipc_link_window ()
  {
    return m_window;
  }
  inline int tipc_link_prio ()
  {
    return m_priority;
  }
  // inline struct sk_buff_head * tipc_link_inputq ()
  // {
  //   return m_inputq;
  // }
  inline char tipc_link_plane ()
  {
    return m_net_plane;
  }
  inline void tipc_link_update_caps (uint16_t capabilities)
  {
    m_peer_caps = capabilities;
  }

  inline void tipc_link_set_mtu (int mtu)
  {
    m_mtu = mtu;
  }
  inline int tipc_link_mtu ()
  {
    return m_mtu;
  }
  inline uint16_t tipc_link_rcv_nxt ()
  {
    return m_rcv_nxt;
  }
  // uint16_t tipc_link_acked()
  // {
  //      return m_acked;
  // }
  inline std::string tipc_link_name ()
  {
    return m_name;
  }
  inline uint32_t tipc_link_state ()
  {
    return m_state;
  }
  /**
 * tipc_link_too_silent - check if link is "too silent"
 * @l: tipc link to be checked
 *
 * Returns true if the link 'silent_intv_cnt' is about to reach the
 * 'abort_limit' value, otherwise false
 */
  inline bool tipc_link_too_silent ()
  {
    return (m_silent_intv_cnt + 2 > m_abort_limit);
  }

  void tipc_link_tnl_prepare (struct tipc_link *tnl,
                              int mtyp, struct sk_buff_head *xmitq);
  void tipc_link_create_dummy_tnl_msg (struct tipc_link *tnl,
                                       struct sk_buff_head *xmitq);
  void tipc_link_build_reset_msg (struct sk_buff_head *xmitq);
  int tipc_link_fsm_evt (int evt);
  void tipc_link_reset ();
  void tipc_link_reset_stats ();
  int tipc_link_xmit (struct sk_buff_head *list,
                      struct sk_buff_head *xmitq);
  struct sk_buff_head * tipc_link_inputq ();
  uint16_t tipc_link_acked ();
  char * tipc_link_name_ext (char *buf);
  bool tipc_link_validate_msg (struct tipc_msg *hdr);
  void tipc_link_set_tolerance (uint32_t tol,
                                struct sk_buff_head *xmitq);
  void tipc_link_set_prio (uint32_t prio,
                           struct sk_buff_head *xmitq);
  void tipc_link_set_queue_limits (uint32_t min_win, uint32_t max_win);
  int __tipc_nl_add_link (struct net *net, struct tipc_nl_msg *msg,
                          int nlflags);
  int tipc_nl_parse_link_prop (struct nlattr *prop, struct nlattr *props[]);
  int tipc_link_timeout (struct sk_buff_head *xmitq);
  int tipc_link_rcv (struct sk_buff *skb,
                     struct sk_buff_head *xmitq);
  int tipc_link_build_state_msg (struct sk_buff_head *xmitq);
  void tipc_link_add_bc_peer (struct tipc_link *snd_l,
                              struct tipc_link *uc_l,
                              struct sk_buff_head *xmitq);
  void tipc_link_remove_bc_peer (struct tipc_link *snd_l,
                                 struct tipc_link *rcv_l,
                                 struct sk_buff_head *xmitq);
  int tipc_link_bc_peers ();
  void tipc_link_bc_ack_rcv (uint16_t acked,
                             struct sk_buff_head *xmitq);
  void tipc_link_build_bc_sync_msg (struct sk_buff_head *xmitq);
  void tipc_link_bc_init_rcv (struct tipc_msg *hdr);
  int tipc_link_bc_sync_rcv (  struct tipc_msg *hdr,
                               struct sk_buff_head *xmitq);
  int tipc_link_bc_nack_rcv (struct sk_buff *skb,
                             struct sk_buff_head *xmitq);

protected:

  virtual void DoDispose (void);
  virtual void DoInitialize (void);
  virtual void NotifyNewAggregate (void);

private:

  Ptr<TipcCore> m_core;

  // Actually this is not useful...
  /**
   * \brief Literal names of TIPC states for use in log messages
   */
  // static const char* const TipcStateName[8];

  struct tipc_stats
  {
    uint32_t sent_pkts;
    uint32_t recv_pkts;
    uint32_t sent_states;
    uint32_t recv_states;
    uint32_t sent_probes;
    uint32_t recv_probes;
    uint32_t sent_nacks;
    uint32_t recv_nacks;
    uint32_t sent_acks;
    uint32_t sent_bundled;
    uint32_t sent_bundles;
    uint32_t recv_bundled;
    uint32_t recv_bundles;
    uint32_t retransmitted;
    uint32_t sent_fragmented;
    uint32_t sent_fragments;
    uint32_t recv_fragmented;
    uint32_t recv_fragments;
    uint32_t link_congs;                /* # port sends blocked by congestion */
    uint32_t deferred_recv;
    uint32_t duplicates;
    uint32_t max_queue_sz;      /* send queue size high water mark */
    uint32_t accu_queue_sz;     /* used for send queue size profiling */
    uint32_t queue_sz_counts;           /* used for send queue size profiling */
    uint32_t msg_length_counts;         /* used for message length profiling */
    uint32_t msg_lengths_total;         /* used for message length profiling */
    uint32_t msg_length_profile[7];     /* used for msg. length profiling */
  };

/**
 * @brief The states of a TIPC link endpoint
 *
 * This code snippet is copy from the source code of TIPC
 *
 * @m_peerAddr: network address of link's peer node
 * @m_name: link name character string
 * @media_addr: media address to use when sending messages over link
 * @timer: link timer
 * @net: pointer to namespace struct
 * @refcnt: reference counter for permanent references (owner node & timer)
 * @peer_session: link session # being used by peer end of link
 * @peer_bearer_id: bearer id used by link's peer endpoint
 * @bearer_id: local bearer id used by link
 * @tolerance: minimum link continuity loss needed to reset link [in ms]
 * @abort_limit: # of unacknowledged continuity probes needed to reset link
 * @m_state: current state of link FSM
 * @peer_caps: bitmap describing capabilities of peer node
 * @silent_intv_cnt: # of timer intervals without any reception from peer
 * @proto_msg: template for control messages generated by link
 * @pmsg: convenience pointer to "proto_msg" field
 * @priority: current link priority
 * @net_plane: current link network plane ('A' through 'H')
 * @mon_state: cookie with information needed by link monitor
 * @backlog_limit: backlog queue congestion thresholds (indexed by importance)
 * @exp_msg_count: # of tunnelled messages expected during link changeover
 * @reset_rcv_checkpt: seq # of last acknowledged message at time of link reset
 * @mtu: current maximum packet size for this link
 * @advertised_mtu: advertised own mtu when link is being established
 * @transmitq: queue for sent, non-acked messages
 * @backlogq: queue for messages waiting to be sent
 * @snt_nxt: next sequence number to use for outbound messages
 * @ackers: # of peers that needs to ack each packet before it can be released
 * @acked: # last packet acked by a certain peer. Used for broadcast.
 * @rcv_nxt: next sequence number to expect for inbound messages
 * @deferred_queue: deferred queue saved OOS b'cast message received from node
 * @unacked_window: # of inbound messages rx'd without ack'ing back to peer
 * @inputq: buffer queue for messages to be delivered upwards
 * @namedq: buffer queue for name table messages to be delivered upwards
 * @next_out: ptr to first unsent outbound message in queue
 * @wakeupq: linked list of wakeup msgs waiting for link congestion to abate
 * @long_msg_seq_no: next identifier to use for outbound fragmented messages
 * @reasm_buf: head of partially reassembled inbound message fragments
 * @bc_rcvr: marks that this is a broadcast receiver link
 * @stats: collects statistics regarding link activity
 * @session: session to be used by link
 * @snd_nxt_state: next send seq number
 * @rcv_nxt_state: next rcv seq number
 * @in_session: have received ACTIVATE_MSG from peer
 * @active: link is active
 * @if_name: associated interface name
 * @rst_cnt: link reset counter
 * @drop_point: seq number for failover handling (FIXME)
 * @failover_reasm_skb: saved failover msg ptr (FIXME)
 * @failover_deferdq: deferred message queue for failover processing (FIXME)
 * @transmq: the link's transmit queue
 * @backlog: link's backlog by priority (importance)
 * @snd_nxt: next sequence number to be used
 * @rcv_unacked: # messages read by user, but not yet acked back to peer
 * @deferdq: deferred receive queue
 * @window: sliding window size for congestion handling
 * @min_win: minimal send window to be used by link
 * @ssthresh: slow start threshold for congestion handling
 * @max_win: maximal send window to be used by link
 * @cong_acks: congestion acks for congestion avoidance (FIXME)
 * @checkpoint: seq number for congestion window size handling
 * @reasm_tnlmsg: fragmentation/reassembly area for tunnel protocol message
 * @last_gap: last gap ack blocks for bcast (FIXME)
 * @last_ga: ptr to gap ack blocks
 * @bc_rcvlink: the peer specific link used for broadcast reception
 * @bc_sndlink: the namespace global link used for broadcast sending
 * @nack_state: bcast nack state
 * @bc_peer_is_up: peer has acked the bcast init msg
 */
  uint32_t m_addr;
  // Used in construct m_name
  std::string m_peer_id;
  uint32_t m_self;
  std::string m_name;
  // struct net *net;

  /* Management and link supervision data */
  uint16_t m_peer_session;
  uint16_t m_session;
  uint16_t m_snd_nxt_state;
  uint16_t m_rcv_nxt_state;
  uint32_t m_peer_bearer_id;
  uint32_t m_bearer_id;
  Time m_tolerance;
  uint32_t m_abort_limit;

  TracedValue<TipcStates_t> m_state;
  Ptr<TipcSignalLinkMonitor> m_monitor;

  uint16_t m_peer_caps;
  bool m_inSession;
  bool m_active;
  uint32_t m_silent_intv_cnt;
  std::string m_if_name;
  uint32_t m_priority;
  char m_net_plane;
  struct tipc_mon_state m_monState;
  uint16_t m_rst_cnt;

  // Below state is unused now
  // TODO: buffer, window, fragmentation...

  /* Failover/synch */
  // uint16_t m_drop_point;
  // struct sk_buff *failover_reasm_skb;
  // struct sk_buff_head failover_deferdq;

  /* Max packet negotiation */
  uint16_t m_mtu;
  uint16_t m_advertised_mtu;

  /* Sending */
  // struct sk_buff_head transmq;
  // struct sk_buff_head backlogq;
  struct
  {
    uint16_t len;
    uint16_t limit;
    // struct sk_buff *target_bskb;
  } m_backlog[5];
  uint16_t m_snd_nxt;

  /* Reception */
  uint16_t m_rcv_nxt;
  uint32_t m_rcv_unacked;
  // struct sk_buff_head deferdq;
  // struct sk_buff_head *inputq;
  // struct sk_buff_head *namedq;
  // Tx buffer management
  Ptr<TipcSignalLinkTxBuffer> m_txBuffer; //!< Tx buffer
  // Rx buffer management
  Ptr<TipcSignalLinkRxBuffer> m_rxBuffer; //!< Rx buffer

  /* Congestion handling */
  // struct sk_buff_head wakeupq;
  uint16_t m_window;
  uint16_t m_min_win;
  uint16_t m_ssthresh;
  uint16_t m_max_win;
  uint16_t m_cong_acks;
  uint16_t m_checkpoint;

  /* Fragmentation/reassembly */
  // struct sk_buff *reasm_buf;
  // struct sk_buff *reasm_tnlmsg;

  /* Broadcast */
  // uint16_t m_ackers;
  // uint16_t m_acked;
  // uint16_t m_last_gap;
  // struct tipc_gap_ack_blks *last_ga;
  // struct tipc_link *bc_rcvlink;
  // struct tipc_link *bc_sndlink;
  // uint8_t m_nack_state;
  // bool m_bc_peer_is_up;

  EventId m_timer{}; //!< timer of the monitor

  /* Statistics */
  struct tipc_stats stats;

/**
 * @brief Perform periodic task as instructed from node timeout
 *
 */
// uint32_t tipc_link_timeout ();
};


} // namespace ns3

#endif // TIPC_SIGNAL_LINK_H
