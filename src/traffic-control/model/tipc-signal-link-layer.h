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
#ifndef TIPC_SIGNAL_LINK_LAYER_H
#define TIPC_SIGNAL_LINK_LAYER_H

#include "ns3/object.h"
#include "ns3/address.h"
#include "ns3/net-device.h"
#include "ns3/traced-value.h"
#include "ns3/node.h"
#include "ns3/queue-item.h"
#include "traffic-control-layer.h"
#include <map>
#include <vector>

namespace ns3 {

class Packet;
class QueueDisc;
class NetDeviceQueueInterface;

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
class TipcSignalLinkLayer : public TrafficControlLayer
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
  TipcSignalLinkLayer ();

  virtual ~TipcSignalLinkLayer ();

  // Delete copy constructor and assignment operator to avoid misuse
  TipcSignalLinkLayer (TipcSignalLinkLayer const &) = delete;
  TipcSignalLinkLayer & operator = (TipcSignalLinkLayer const &) = delete;

  /**
   * \ingroup tipc
   * \brief Names of the 7 TIPC states
   *
   */
  typedef enum
  {
    LINK_ESTABLISHED     = 0xe,
    LINK_ESTABLISHING    = 0xe  << 4,
    LINK_RESET           = 0x1  << 8,
    LINK_RESETTING       = 0x2  << 12,
    LINK_PEER_RESET      = 0xd  << 16,
    LINK_FAILINGOVER     = 0xf  << 20,
    LINK_SYNCHING        = 0xc  << 24,
    LAST_STATE    /**< Last state, used only in debug messages                */
  } TipcStates_t;

protected:

  virtual void DoDispose (void);
  virtual void DoInitialize (void);
  virtual void NotifyNewAggregate (void);

private:

  // Actually this is not useful...
  /**
   * \brief Literal names of TIPC states for use in log messages
   */
  // static const char* const TipcStateName[8];

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
 * @addr: network address of link's peer node
 * @name: link name character string
 * @media_addr: media address to use when sending messages over link
 * @timer: link timer
 * @net: pointer to namespace struct
 * @refcnt: reference counter for permanent references (owner node & timer)
 * @peer_session: link session # being used by peer end of link
 * @peer_bearer_id: bearer id used by link's peer endpoint
 * @bearer_id: local bearer id used by link
 * @tolerance: minimum link continuity loss needed to reset link [in ms]
 * @abort_limit: # of unacknowledged continuity probes needed to reset link
 * @state: current state of link FSM
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
  // char m_name[TIPC_MAX_LINK_NAME];
  // struct net *net;

  /* Management and link supervision data */
  uint16_t m_peer_session;
  uint16_t m_session;
  uint16_t m_snd_nxt_state;
  uint16_t m_rcv_nxt_state;
  uint32_t m_peer_bearer_id;
  uint32_t m_bearer_id;
  uint32_t m_tolerance;
  uint32_t m_abort_limit;

  TracedValue<TipcStates_t> m_state;

  uint16_t m_peer_caps;
  bool m_in_session;
  bool m_active;
  uint32_t m_silent_intv_cnt;
  // char m_if_name[TIPC_MAX_IF_NAME];
  uint32_t m_priority;
  char m_net_plane;
  // struct tipc_mon_state mon_state;
  uint16_t m_rst_cnt;

  // Below state is unused now
  // TODO: buffer, window, fragmentation...

  /* Failover/synch */
  // uint16_t m_drop_point;
  // struct sk_buff *failover_reasm_skb;
  // struct sk_buff_head failover_deferdq;

  /* Max packet negotiation */
  // uint16_t m_mtu;
  // uint16_t m_advertised_mtu;

  /* Sending */
  // struct sk_buff_head transmq;
  // struct sk_buff_head backlogq;
  // struct
  // {
  //   uint16_t m_len;
  //   uint16_t m_limit;
  //   struct sk_buff *target_bskb;
  // } backlog[5];
  // uint16_t m_snd_nxt;

  /* Reception */
  // uint16_t m_rcv_nxt;
  // uint32_t m_rcv_unacked;
  // struct sk_buff_head deferdq;
  // struct sk_buff_head *inputq;
  // struct sk_buff_head *namedq;

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

  /* Statistics */
  struct tipc_stats stats;

/**
 * \brief Link finite state machine
 *
 * \param event state machine event to be processed
 */
  uint32_t LinkFsmEvent (int evt);
};

/**
 * \ingroup tipc
 * TracedValue Callback signature for TIPC state trace
 *
 * \param [in] oldValue original value of the traced variable
 * \param [in] newValue new value of the traced variable
 */
typedef void (* TipcStatesTracedValueCallback)(const TipcSignalLinkLayer::TipcStates_t oldValue,
                                               const TipcSignalLinkLayer::TipcStates_t newValue);

} // namespace ns3

#endif // TIPC_SIGNAL_LINK_LAYER_H
