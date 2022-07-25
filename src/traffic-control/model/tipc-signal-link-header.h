/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005 INRIA
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
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef TIPC_SIGNAL_LINK_HEADER_H
#define TIPC_SIGNAL_LINK_HEADER_H

#include <stdint.h>
#include <string>
#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/sequence-number.h"

namespace ns3 {
/*
 * Limiting values for messages
 */
#define TIPC_MAX_USER_MSG_SIZE	66000U
/*
* Constants and routines used to read and write TIPC payload message headers
*
* Note: Some items are also used with TIPC internal message headers
*/
#define TIPC_VERSION              2

/*
 * Payload message users are defined in TIPC's public API:
 * - TIPC_LOW_IMPORTANCE
 * - TIPC_MEDIUM_IMPORTANCE
 * - TIPC_HIGH_IMPORTANCE
 * - TIPC_CRITICAL_IMPORTANCE
 */
#define TIPC_SYSTEM_IMPORTANCE  4
/*
 * Payload message types
 */
#define TIPC_CONN_MSG           0
#define TIPC_MCAST_MSG          1
#define TIPC_NAMED_MSG          2
#define TIPC_DIRECT_MSG         3
#define TIPC_GRP_MEMBER_EVT     4
#define TIPC_GRP_BCAST_MSG      5
#define TIPC_GRP_MCAST_MSG      6
#define TIPC_GRP_UCAST_MSG      7
/*
 * Internal message users
 */
#define  BCAST_PROTOCOL       5
#define  MSG_BUNDLER          6
#define  LINK_PROTOCOL        7
#define  CONN_MANAGER         8
#define  GROUP_PROTOCOL       9
#define  TUNNEL_PROTOCOL      10
#define  NAME_DISTRIBUTOR     11
#define  MSG_FRAGMENTER       12
#define  LINK_CONFIG          13
#define  SOCK_WAKEUP          14       /* pseudo user */
#define  TOP_SRV              15       /* pseudo user */
/*
 * Message header sizes
 */
#define SHORT_H_SIZE              24    /* In-cluster basic payload message */
#define BASIC_H_SIZE              32    /* Basic payload message */
#define NAMED_H_SIZE              40    /* Named payload message */
#define MCAST_H_SIZE              44    /* Multicast payload message */
#define GROUP_H_SIZE              44    /* Group payload message */
#define INT_H_SIZE                40    /* Internal messages */
#define MIN_H_SIZE                24    /* Smallest legal TIPC header size */
#define MAX_H_SIZE                60    /* Largest possible TIPC header size */
#define MAX_MSG_SIZE (MAX_H_SIZE + TIPC_MAX_USER_MSG_SIZE)
#define FB_MTU                  3744
#define TIPC_MEDIA_INFO_OFFSET  5

/**
 * \ingroup udp
 * \brief Packet header for UDP packets
 *
 * This class has fields corresponding to those in a network UDP header
 * (port numbers, payload size, checksum) as well as methods for serialization
 * to and deserialization from a byte buffer.
 */
class TipcSignalLinkHeader : public Header
{
public:

  /**
   * \brief Constructor
   *
   * Creates a null header
   */
  TipcSignalLinkHeader ();
  ~TipcSignalLinkHeader ();

  // the getters and setters are easy to understand, we may not comment them
  /**
   * \param node the destination node for this TipcSignalLinkHeader
   */
  void SetDestinationNode (uint16_t node);
  /**
   * \return the destination node for this TipcSignalLinkHeader
   */
  uint16_t GetDestinationNode (void) const;

  void SetOriginatingNode (uint16_t node);
  uint16_t GetOriginatingNode (void) const;
  /**
 * \brief Get the sequence number
 * \return the sequence number for this TcpHeader
 */
  SequenceNumber32 GetSequenceNumber () const;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  uint32_t m_word0;           //!< vers|msg usr|hdr sz|n|resrv|packet size

  uint16_t m_word1h;          //!< m typ|sequence gap
  uint16_t m_broadcastAckNo;  //!< Broadcast Ack No

  uint16_t m_word2h;          //!< link level ack no/bc gap after
  uint16_t m_word2l;          //!< link level/bc seqno/bc gap to

  uint32_t m_previousNode;    //!< Previous Node

  uint16_t m_word4h;          //!< last sent broadcast/fragm no
  uint16_t m_word4l;          //!< next sent pkt/fragm msg no

  uint16_t m_sessionNo;       //!< Session No
  uint16_t m_word5l;          //!< res|r|berid|link prio|netpl|p

  uint32_t m_originatingNode; //!< Originating Node
  uint32_t m_destinationNode; //!< Destination Node
  uint32_t m_transSeqNumber;  //!< Transport Sequence Number

  uint16_t m_word9h;          //!< Msg Count / Max Packet
  uint16_t m_linkTolerance;   //!< Link Tolerance
};

} // namespace ns3

#endif /* UDP_HEADER */
