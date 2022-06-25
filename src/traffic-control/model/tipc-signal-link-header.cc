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

#include "tipc-signal-link-header.h"
#include "ns3/address-utils.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TipcSignalLinkHeader");

NS_OBJECT_ENSURE_REGISTERED (TipcSignalLinkHeader);

/* The magic values below are used only for debugging.
 * They can be used to easily detect memory corruption
 * problems so you can see the patterns in memory.
 */
TipcSignalLinkHeader::TipcSignalLinkHeader ()
  : m_word0 (0xfffffffd),
    m_word1h (0xfffd),
    m_broadcastAckNo (0xfffd),
    m_word2h (0xfffd),
    m_word2l (0xfffd),
    m_previousNode (0xfffffffd),
    m_word4h (0xfffd),
    m_word4l (0xfffd),
    m_sessionNo (0xfffd),
    m_word5l (0xfffd),
    m_originatingNode (0xfffffffd),
    m_destinationNode (0xfffffffd),
    m_transSeqNumber (0xfffffffd),
    m_word9h (0xfffd),
    m_linkTolerance (0xfffd)
{}
TipcSignalLinkHeader::~TipcSignalLinkHeader ()
{
  m_word0 = 0xfffffffe;
  m_word1h = 0xfffe;
  m_broadcastAckNo = 0xfffe;
  m_word2h = 0xfffe;
  m_word2l = 0xfffe;
  m_previousNode = 0xfffffffe;
  m_word4h = 0xfffe;
  m_word4l = 0xfffe;
  m_sessionNo = 0xfffe;
  m_word5l = 0xfffe;
  m_originatingNode = 0xfffffffe;
  m_destinationNode = 0xfffffffe;
  m_transSeqNumber = 0xfffffffe;
  m_word9h = 0xfffe;
  m_linkTolerance = 0xfffe;
}

void
TipcSignalLinkHeader::SetDestinationNode (uint16_t node)
{
  m_destinationNode = node;
}
uint16_t
TipcSignalLinkHeader::GetDestinationNode (void) const
{
  return m_destinationNode;
}

void
TipcSignalLinkHeader::SetOriginatingNode (uint16_t node)
{
  m_originatingNode = node;
}
uint16_t
TipcSignalLinkHeader::GetOriginatingNode (void) const
{
  return m_originatingNode;
}

TypeId
TipcSignalLinkHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TipcSignalLinkHeader")
    .SetParent<Header> ()
    .SetGroupName ("Tipc")
    .AddConstructor<TipcSignalLinkHeader> ()
  ;
  return tid;
}

TypeId
TipcSignalLinkHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
TipcSignalLinkHeader::Print (std::ostream &os) const
{
  os << "The print format of TipcSignalLinkHeader is not writen now"
  ;
}

uint32_t
TipcSignalLinkHeader::GetSerializedSize (void) const
{
  return 20;
}

void
TipcSignalLinkHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteHtonU32 (m_word0);

  i.WriteHtonU16 (m_word1h);
  i.WriteHtonU16 (m_broadcastAckNo);

  i.WriteHtonU16 (m_word2h);
  i.WriteHtonU16 (m_word2l);

  i.WriteHtonU32 (m_previousNode);

  i.WriteHtonU16 (m_word4h);
  i.WriteHtonU16 (m_word4l);

  i.WriteHtonU16 (m_sessionNo);
  i.WriteHtonU16 (m_word5l);

  i.WriteHtonU32 (m_originatingNode);
  i.WriteHtonU32 (m_destinationNode);
  i.WriteHtonU32 (m_transSeqNumber);

  i.WriteHtonU16 (m_word9h);
  i.WriteHtonU16 (m_linkTolerance);
}

uint32_t
TipcSignalLinkHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  m_word0 = i.ReadNtohU32 ();

  m_word1h = i.ReadNtohU16 ();
  m_broadcastAckNo = i.ReadNtohU16 ();

  m_word2h = i.ReadU16 ();
  m_word2l = i.ReadU16 ();

  m_previousNode = i.ReadNtohU32 ();

  m_word4h = i.ReadU16 ();
  m_word4l = i.ReadU16 ();

  m_sessionNo = i.ReadU16 ();
  m_word5l = i.ReadU16 ();

  m_originatingNode = i.ReadNtohU32 ();
  m_destinationNode = i.ReadNtohU32 ();
  m_transSeqNumber = i.ReadNtohU32 ();


  m_word9h = i.ReadU16 ();
  m_linkTolerance = i.ReadU16 ();

  return GetSerializedSize ();
}

} // namespace ns3
