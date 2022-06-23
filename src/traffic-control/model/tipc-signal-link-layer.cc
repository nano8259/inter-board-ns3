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

#include "tipc-signal-link-layer.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/log.h"
#include "ns3/object-map.h"
#include "ns3/packet.h"
#include "ns3/socket.h"
#include "ns3/queue-disc.h"
#include <tuple>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TipcSignalLinkLayer");

NS_OBJECT_ENSURE_REGISTERED (TipcSignalLinkLayer);

TypeId
TipcSignalLinkLayer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TipcSignalLinkLayer")
    .SetParent<TrafficControlLayer> ()
    .SetGroupName ("Tipc")
    .AddConstructor<TipcSignalLinkLayer> ()
  ;
  return tid;
}

TypeId
TipcSignalLinkLayer::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

TipcSignalLinkLayer::TipcSignalLinkLayer ()
  : TrafficControlLayer ()
{
  NS_LOG_FUNCTION (this);
}

TipcSignalLinkLayer::~TipcSignalLinkLayer ()
{
  NS_LOG_FUNCTION (this);
}

void
TipcSignalLinkLayer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  // TODO: destruct logic here
  TrafficControlLayer::DoDispose ();
}

void
TipcSignalLinkLayer::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  // TODO: initialize logic here
  TrafficControlLayer::DoInitialize ();
}

void
TipcSignalLinkLayer::NotifyNewAggregate ()
{
  NS_LOG_FUNCTION (this);
  TrafficControlLayer::NotifyNewAggregate ();
}

} // namespace ns3
