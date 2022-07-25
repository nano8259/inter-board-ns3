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
#include "tipc-signal-link-node.h"
#include "tipc-signal-link.h"
#include "tipc-signal-link-monitor.h"
#include <map>
#include <vector>

namespace ns3 {

class Packet;
class QueueDisc;
class NetDeviceQueueInterface;

/**
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

protected:

  virtual void DoDispose (void);
  virtual void DoInitialize (void);
  virtual void NotifyNewAggregate (void);

private:


};


} // namespace ns3

#endif // TIPC_SIGNAL_LINK_LAYER_H
