/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Belgrade, Faculty of Transport and Traffic Engineering
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
 * Authors: Nenad Jevtic <n.jevtic@sf.bg.ac.rs>
 *                       <nen.jevtic@gmail.com>
 *          Marija Malnar <m.malnar@sf.bg.ac.rs>
 */
 
#include "ns3/stats-data.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/nstime.h"
#include "stats-header.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("StatsData");

FlowData::FlowData (NetFlowId fid, std::string fn, bool singleFile) 
    : m_flowId (fid),
      m_fileName (fn),
      m_fileNamePrefix ("Stats"),
      m_delayVector ("Delay [us]"),
      m_fileWriteEnable (true),
      m_memoryWriteEnable (false)
{
  NS_LOG_FUNCTION (this);
  if (m_fileWriteEnable)
  {
    // Create authomatic name based on flow info
    // not posible if user want to write all data flows to single file
    if (m_fileName == "noname" && !singleFile) 
      {
        std::ostringstream oss;
        oss << m_fileNamePrefix 
            << "-Flow_" << m_flowId.index 
            << "-SourceNode_" << m_flowId.sourceNodeId 
            << "-SourceApp_" << m_flowId.sourceAppId
            << "-SinkNode_" << m_flowId.sinkNodeId 
            << "-SinkApp_" << m_flowId.sinkAppId
            << ".csv"; 
        m_fileName = oss.str ();
      }
    // If first (index==0) flow then write file header
    if (m_flowId.index == 0)
    {
      m_delayVector.WriteFileHeader (m_fileName);
    }
    // If not first (index>0) flow then write file header only for writing to multiple files
    else if (!singleFile)
    {
      m_delayVector.WriteFileHeader (m_fileName);
    }
  }
}

void 
FlowData::PacketReceived (Ptr<const Packet> packet, bool singleFile)
{
  NS_LOG_FUNCTION (this);
  StatsHeader statsHeader;
  packet->PeekHeader (statsHeader);
  //uint32_t sourceNodeId = statsHeader.GetNodeId ();
  //uint32_t sourceAppId = statsHeader.GetApplicationId ();
  
  // Scalar data
  m_scalarData.totalRxPackets++; // number of received packets
  m_scalarData.packetSizeInBytes = packet->GetSize (); // last packet's size
  m_scalarData.totalRxBytes += m_scalarData.packetSizeInBytes; // total bytes received
  uint32_t currentSequenceNumber = statsHeader.GetSeq (); // SeqNo is counting from 0, so (SeqNo + 1) is equal to the number of packets sent
  m_scalarData.totalTxPackets = ((currentSequenceNumber+1) > m_scalarData.totalTxPackets) ? (currentSequenceNumber+1) : (m_scalarData.totalTxPackets);
  m_scalarData.lastPacketReceived = Simulator::Now ();
  m_scalarData.lastPacketSent = statsHeader.GetTs ();
  m_scalarData.lastDelay = m_scalarData.lastPacketReceived - m_scalarData.lastPacketSent;
  if (m_scalarData.totalRxPackets == 1) // first received packet
  {
    m_scalarData.firstPacketReceived = m_scalarData.lastPacketReceived;
    m_scalarData.firstPacketSent = m_scalarData.lastPacketSent; // Warning: actual first packet sent can be lost
    m_scalarData.firstDelay = m_scalarData.lastDelay; // Warning: actual first packet sent can be lost
  }

  // vector data
  if (IsFileWriteEnabled ()) m_delayVector.WriteValueToFile (m_fileName, m_scalarData.lastPacketReceived, m_scalarData.lastDelay, singleFile, m_flowId.index, currentSequenceNumber);
  if (IsMemoryWriteEnabled ()) m_delayVector.AddValueToVector (m_scalarData.lastPacketReceived, m_scalarData.lastDelay); 
}


void
FlowData::Finalize (bool singleFile, uint32_t allRxPackets)
{
  NS_LOG_FUNCTION (this);
  if (m_fileWriteEnable)
  {
    std::ofstream out (m_fileName.c_str (), std::ios::app);
    out << std::endl;
    out << "Flow Index,Source Node,Source App,Sink Node,Sink App" << std::endl;
    out << m_flowId.index << "," << m_flowId.sourceNodeId << "," << m_flowId.sourceAppId << "," << m_flowId.sinkNodeId << "," << m_flowId.sinkAppId << std::endl;
    
    uint32_t row;
    char column;
    if (singleFile)
    {
      row = allRxPackets + 1;
      column = 'D' + (char) (m_flowId.index);
    }
    else
    {
      row = m_scalarData.totalRxPackets + 1;
      column = 'D';
    }
    
    out << "Number of packets for flow," << m_scalarData.totalRxPackets << "," << m_delayVector.GetNValuesWrittenToFile () << std::endl;

    out << std::endl;

    out << "E2E average delay [us]," << "=SUM(" << column << "2:" << column << row << ")/" << m_scalarData.totalRxPackets << std::endl;
    out << "E2E median delay [us]," << "=MEDIAN(" << column << "2:" << column << row << ")" << std::endl;
    out << "E2E max delay [us]," << "=MAX(" << column << "2:" << column << row << ")"  << std::endl;
    out << "Jitter [us]," << "=SQRT((SUMSQ(" << column << "2:" << column << row << ")-(SUM(" << column << "2:" << column << row << ")/" << m_scalarData.totalRxPackets << ")^2)/" << m_scalarData.totalRxPackets -1 << ")" << std::endl;
    
    out << std::endl;

    out << "Rx," << "First packet [us]:," << m_scalarData.firstPacketReceived.GetDouble () / 1000.0 << std::endl;
    out << "Rx,"<< "Last packet [us]:," << m_scalarData.lastPacketReceived.GetDouble () / 1000. << std::endl;
    Time diffRx = m_scalarData.lastPacketReceived - m_scalarData.firstPacketReceived;
    out << "Rx,"<< "Duration of sending packets [s]:,"  << diffRx.GetSeconds () << std::endl;
    out << "Rx,"<< "Count of packets:,"  << m_scalarData.totalRxPackets << std::endl;
    out << "Rx,"<< "Bytes:,"  << m_scalarData.totalRxBytes << std::endl;
    if (diffRx.GetSeconds ())
    {
      out << "Rx,"<< "Throughput [bps]:,"  << (double)m_scalarData.totalRxBytes * 8.0 / diffRx.GetSeconds () << std::endl;
    }

    out << std::endl;

    out << "Tx," << "First packet [us]:," << m_scalarData.firstPacketSent.GetDouble () / 1000.0 << std::endl;
    out << "Tx,"<< "Last packet [us]:," << m_scalarData.lastPacketSent.GetDouble () / 1000. << std::endl;
    Time diffTx = m_scalarData.lastPacketSent - m_scalarData.firstPacketSent;
    out << "Tx,"<< "Duration of sending packets [s]:,"  << diffTx.GetSeconds () << std::endl;
    out << "Tx,"<< "Count of packets:,"  << m_scalarData.totalTxPackets << std::endl;
    out << "Tx,"<< "Bytes:,"  << (m_scalarData.totalTxPackets*m_scalarData.packetSizeInBytes) << std::endl;
    if (diffTx.GetSeconds ())
    {
      out << "Tx,"<< "Throughput [bps]:,"  << (double)(m_scalarData.totalTxPackets*m_scalarData.packetSizeInBytes) * 8.0 / diffTx.GetSeconds () << std::endl;
    }

    out << std::endl;

    out << ",Lost packets:,"  << m_scalarData.totalTxPackets - m_scalarData.totalRxPackets << std::endl;

    if ((m_scalarData.lastPacketReceived - m_scalarData.firstPacketSent).GetSeconds ())
    {
      out << ",Real throughput [bps]:," << (double)m_scalarData.totalRxBytes * 8.0 / (m_scalarData.lastPacketReceived - m_scalarData.firstPacketSent).GetSeconds () << std::endl;
    }

    out.close ();
  }
}


void
StatsFlows::PacketReceived (Ptr<const Packet> packet, uint32_t sinkNodeId, uint32_t sinkAppId)
{
  NS_LOG_FUNCTION (this);
  m_allRxPackets++;
  NS_LOG_INFO ("Stigao paket: " << m_allRxPackets);
  StatsHeader statsHeader;
  packet->PeekHeader (statsHeader);
  uint32_t sourceNodeId = statsHeader.GetNodeId ();
  uint32_t sourceAppId = statsHeader.GetApplicationId ();
  NetFlowId fid (sourceNodeId, sourceAppId, sinkNodeId, sinkAppId);
  
  uint16_t i;
  for (i = 0; i < m_flowIds.size(); i++)
  {
    if (fid == m_flowIds[i]) break;
  }
  
  if (i == m_flowIds.size())
  {
    NS_LOG_INFO (">>>>>>>>>>>>>>  Novi Flow!!! >>>>>>>>>>>>>>>>>>>>>");
    fid.index = i;
    m_flowIds.push_back (fid);
    FlowData fd (fid, m_fileName, m_singleFile);
    m_flowData.push_back (fd);
    NS_LOG_INFO ("Novi flow: " << m_flowData[i].GetFlowId ().index 
            << "-SourceNode_" << m_flowData[i].GetFlowId ().sourceNodeId 
            << "-SourceApp_" << m_flowData[i].GetFlowId ().sourceAppId
            << "-SinkNode_" << m_flowData[i].GetFlowId ().sinkNodeId 
            << "-SinkApp_" << m_flowData[i].GetFlowId ().sinkAppId);
  }
  else
  {
    NS_LOG_INFO ("Nadjen flow: " << m_flowIds[i].index 
            << "-SourceNode_" << m_flowIds[i].sourceNodeId 
            << "-SourceApp_" << m_flowIds[i].sourceAppId
            << "-SinkNode_" << m_flowIds[i].sinkNodeId 
            << "-SinkApp_" << m_flowIds[i].sinkAppId);
  }
  NS_LOG_INFO ("i=" << i << ", sizeFlowData=" << m_flowData.size () << ", sizeFlowId=" << m_flowIds.size ());
  m_flowData[i].PacketReceived (packet, m_singleFile);
}

void
StatsFlows::Finalize ()
{
  NS_LOG_FUNCTION (this);
  
  for (uint16_t i = 0; i < m_flowData.size(); i++)
  {
    NS_LOG_INFO ("FINALIZE: call Finalize() for flowId=" << i);
    m_flowData[i].Finalize (m_singleFile, m_allRxPackets);
  }
  
  // Stats of all flows
  int lastRow = m_allRxPackets + 1;
  int flowDataRaws = 26;
  int nFlows = m_flowIds.size ();

  std::ofstream out (m_fileName.c_str (), std::ios::app);
  
  out << std::endl;
  out << "AVERAGE RESULTS FOR ALL FLOWS" << std::endl;

  // Average e2ed
  out << "Average E2E Delay [ms]:,=(B" << lastRow+6;
  for (int i=1; i<nFlows; i++)
  {
    out << "+B" << lastRow+i*flowDataRaws+6;
  }
  out << ")/1000/" << nFlows << std::endl;
  
  
  // Median e2ed
  out << "Median E2E Delay [ms]:,=(B" << lastRow+7;
  for (int i=1; i<nFlows; i++)
  {
    out << "+B" << lastRow+i*flowDataRaws+7;
  }
  out << ")/1000/" << nFlows << std::endl;
  
  // Max e2ed
  out << "Max of E2E Delay [ms]:,=(B" << lastRow+8;
  for (int i=1; i<nFlows; i++)
  {
    out << "+B" << lastRow+i*flowDataRaws+8;
  }
  out << ")/1000/" << nFlows << std::endl;
  
  // Jitter e2ed
  out << "Jitter of E2E Delay [ms]:,=(B" << lastRow+9;
  for (int i=1; i<nFlows; i++)
  {
    out << "+B" << lastRow+i*flowDataRaws+9;
  }
  out << ")/1000/" << nFlows << std::endl;
  
  // Transmitted packets (based on sequence number)
  out << "Number of all Tx packets:,=C" << lastRow+21;
  for (int i=1; i<nFlows; i++)
  {
    out << "+C" << lastRow+i*flowDataRaws+21;
  }
  out << std::endl;

  // Received packets
  out << "Number of all Rx packets:," << m_allRxPackets << std::endl;
  
  // Lost
  out << "Number of all lost packets:,=";
  std::ostringstream oss;
  oss << "C" << lastRow+25;
  for (int i=1; i<nFlows; i++)
  {
    oss << "+C" << lastRow+i*flowDataRaws+25;
  }
  out << oss.str () << std::endl;
  
  // Lost %
  out << "Lost packets [%]:,=100*(" << oss.str () << ")/(" << m_allRxPackets << "+" << oss.str () << ")" << std::endl;
  
  // Troughput
  out << "Real troughput [kbps]:,=(C" << lastRow+26;
  for (int i=1; i<nFlows; i++)
  {
    out << "+C" << lastRow+i*flowDataRaws+26;
  }
  out << ")/1000/" << nFlows << std::endl;
  
  out.close ();
}


} // namespace ns3
