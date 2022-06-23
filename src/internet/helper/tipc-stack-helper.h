/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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

#ifndef TIPC_STACK_HELPER_H
#define TIPC_STACK_HELPER_H

#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv6-l3-protocol.h"
#include "internet-stack-helper.h"

namespace ns3 {

class Node;
class Ipv4RoutingHelper;
class Ipv6RoutingHelper;

/**
 * \ingroup tipc
 *
 * \brief aggregate TIPC functionality to existing Nodes.
 *
 * This helper enables pcap and ascii tracing of events in the internet stack
 * associated with a node.  This is substantially similar to the tracing
 * that happens in device helpers, but the important difference is that, well,
 * there is no device.  This means that the creation of output file names will
 * change, and also the user-visible methods will not reference devices and
 * therefore the number of trace enable methods is reduced.
 *
 * Normally we avoid multiple inheritance in ns-3, however, the classes 
 * PcapUserHelperForIpv4 and AsciiTraceUserHelperForIpv4 are
 * treated as "mixins".  A mixin is a self-contained class that
 * encapsulates a general attribute or a set of functionality that
 * may be of interest to many other classes.
 *
 * This class aggregates instances of these objects, by default, to each node:
 *  - ns3::ArpL3Protocol
 *  - ns3::Ipv4L3Protocol
 *  - ns3::Icmpv4L4Protocol
 *  - ns3::Ipv6L3Protocol
 *  - ns3::Icmpv6L4Protocol
 *  - ns3::UdpL4Protocol
 *  - ns3::TrafficControlLayer
 *  - a TCP based on the TCP factory provided
 *  - a PacketSocketFactory
 *  - Ipv4 routing (a list routing object, a global routing object, and a static routing object)
 *  - Ipv6 routing (a static routing object)
 */
class TipcStackHelper : public InternetStackHelper
{
public:
  /**
   * Create a new InternetStackHelper which uses a mix of static routing
   * and global routing by default. The static routing protocol 
   * (ns3::Ipv4StaticRouting) and the global routing protocol are
   * stored in an ns3::Ipv4ListRouting protocol with priorities 0, and -10
   * by default. If you wish to use different priorites and different
   * routing protocols, you need to use an adhoc ns3::Ipv4RoutingHelper, 
   * such as ns3::OlsrHelper
   */
  TipcStackHelper(void);

  /**
   * Destroy the InternetStackHelper
   */
  virtual ~TipcStackHelper(void);

  /**
   * \brief Copy constructor
   * \param o Object to copy from.
   */
  TipcStackHelper (const TipcStackHelper &o);

  /**
   * \brief Copy constructor
   * \param o Object to copy from.
   * \returns A copy of the InternetStackHelper.
   */
  TipcStackHelper &operator = (const TipcStackHelper &o);

  // used to avoid hiding other functions with the same name in the base class
  using InternetStackHelper::Install;

  /**
   * Aggregate implementations of the ns3::Ipv4, ns3::Ipv6, ns3::Udp, and ns3::Tcp classes
   * onto the provided node.  This method will assert if called on a node that 
   * already has an Ipv4 object aggregated to it.
   * 
   * \param node The node on which to install the stack.
   */
  void Install (Ptr<Node> node) const;

private:

  // /**
  //  * \brief Initialize the helper to its default values
  //  */
  // void Initialize (void);
};

} // namespace ns3

#endif /* INTERNET_STACK_HELPER_H */
