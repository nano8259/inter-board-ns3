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

#include "tipc-core.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/log.h"
#include "ns3/object-map.h"
#include "ns3/packet.h"
#include "ns3/socket.h"
#include "ns3/queue-disc.h"
#include "ns3/random-variable-stream.h"
// #include <tuple>
#include <sstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TipcCore");

NS_OBJECT_ENSURE_REGISTERED (TipcCore);

// The address is start at 1, 0 is reserved as error value
uint32_t TipcCore::global_node_addr = 1;

TypeId
TipcCore::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TipcCore")
    .SetParent<TrafficControlLayer> ()
    .SetGroupName ("Tipc")
    .AddConstructor<TipcCore> ()
  ;
  return tid;
}

TypeId
TipcCore::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

TipcCore::TipcCore ()
  : Object ()
{
  NS_LOG_FUNCTION (this);
  m_net_id = 4711; // don't know why
  m_node_addr = global_node_addr++;
  m_mon_threshold = TIPC_DEF_MON_THRESHOLD;
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  m_random = x->GetInteger ();
  sprintf(reinterpret_cast<char *>(m_node_id), "%x", m_node_addr);
  sprintf(m_node_id_string, "%x", m_node_addr);
}

TipcCore::~TipcCore ()
{
  NS_LOG_FUNCTION (this);
}

void
TipcCore::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  // TODO: destruct logic here
  Object::DoDispose ();
}

void
TipcCore::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  // TODO: initialize logic here
  Object::DoInitialize ();
}

void
TipcCore::NotifyNewAggregate ()
{
  NS_LOG_FUNCTION (this);
  Object::NotifyNewAggregate ();
}


} // namespace ns3
