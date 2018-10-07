/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 INRIA
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

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "stats-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("StatsHeader");

NS_OBJECT_ENSURE_REGISTERED (StatsHeader);

StatsHeader::StatsHeader ()
  : m_seq (0),
    m_ts (Simulator::Now ().GetTimeStep ()),
    m_nodeId (0),
    m_appId (0)
{
  NS_LOG_FUNCTION (this);
}

void
StatsHeader::SetSeq (uint32_t seq)
{
  NS_LOG_FUNCTION (this << seq);
  m_seq = seq;
}
uint32_t
StatsHeader::GetSeq (void) const
{
  NS_LOG_FUNCTION (this);
  return m_seq;
}

Time
StatsHeader::GetTs (void) const
{
  NS_LOG_FUNCTION (this);
  return TimeStep (m_ts);
}

void
StatsHeader::SetNodeId (uint32_t nodeId)
{
  NS_LOG_FUNCTION (this << m_nodeId);
  m_nodeId = nodeId;
}

uint32_t
StatsHeader::GetNodeId (void) const
{
  NS_LOG_FUNCTION (this);
  return m_nodeId;
}

void
StatsHeader::SetApplicationId (uint32_t appId)
{
  NS_LOG_FUNCTION (this << m_appId);
  m_appId = appId;
}

uint32_t
StatsHeader::GetApplicationId (void) const
{
  NS_LOG_FUNCTION (this);
  return m_appId;
}

TypeId
StatsHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::StatsHeader")
    .SetParent<Header> ()
    .SetGroupName("Applications")
    .AddConstructor<StatsHeader> ()
  ;
  return tid;
}
TypeId
StatsHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
StatsHeader::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "(seq=" << m_seq << " time=" << TimeStep (m_ts).GetSeconds () << " nodeId=" << m_nodeId << " appId=" << m_appId << ")";
}
uint32_t
StatsHeader::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 4+8+4+4;
}

void
StatsHeader::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  i.WriteHtonU32 (m_seq);
  i.WriteHtonU64 (m_ts);
  i.WriteHtonU32 (m_nodeId);
  i.WriteHtonU32 (m_appId);
}
uint32_t
StatsHeader::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  m_seq = i.ReadNtohU32 ();
  m_ts = i.ReadNtohU64 ();
  m_nodeId = i.ReadNtohU32 ();
  m_appId = i.ReadNtohU32 ();
  return GetSerializedSize ();
}

} // namespace ns3
