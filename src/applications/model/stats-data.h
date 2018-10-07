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

#ifndef STATS_DATA_H
#define STATS_DATA_H
#include <string>
#include <vector>
#include <fstream>
#include <utility> // std::pair
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"

namespace ns3 {

struct ScalarData
{
  ScalarData () : totalRxPackets (0), totalTxPackets (0), totalRxBytes (0) {};
  uint32_t totalRxPackets; // number of total packets receiced
  uint32_t totalTxPackets; // number of total packets sent
  uint64_t totalRxBytes;   // total received bytes
  uint16_t packetSizeInBytes;
  Time firstPacketSent, lastPacketSent;
  Time firstPacketReceived, lastPacketReceived;
  Time firstDelay, lastDelay;
};

template<class T>
class VectorData
{
public:
  VectorData (std::string name = "vector value") 
    : m_name (name),
      m_numValuesWrittenToFile (0)
  {};
  void AddValueToVector (Time time, T t)
  {
    m_vd.push_back(std::make_pair (time, t));
  };
  void WriteValueToFile (std::string fileName, Time time, T t, bool singleFile = false, uint16_t flowIndex = 0, uint32_t seqNo = 0);
  void WriteFileHeader (std::string fileName);
  int GetNValuesWrittenToFile () {return m_numValuesWrittenToFile; };
  int GetNValuesWrittenToMemory () {return m_vd.size (); };
private:
  std::string m_name;
  int m_numValuesWrittenToFile;
  std::vector<std::pair<Time,T>> m_vd;
};

template<class T>
void VectorData<T>::WriteValueToFile (std::string fileName, Time time, T t, bool singleFile, uint16_t flowIndex, uint32_t seqNo)
{
  std::ofstream out (fileName.c_str (), std::ios::app);
  
  out << flowIndex << ",";
  out << time.GetDouble () / 1000.0 << ",";
  out << seqNo << ",";
  
  if (singleFile)
  {
    for (int i=0; i < flowIndex; ++i)
    {
      out << ",";
    }
  }  
  
  Time *tp = dynamic_cast<Time*> (&t);
  if (tp != 0)
  { // if T is actually Time print time in mikroseconds
    out << t.GetDouble () / 1000.0 << std::endl;  
  }
  else // class T must have defined operator<<
  {
    out << t << std::endl; 
  }
  out.close ();
  m_numValuesWrittenToFile++;
}

template<class T>
void VectorData<T>::WriteFileHeader (std::string fileName)
{
  std::ofstream outHeader (fileName.c_str ());
  outHeader << "Flow Index,Time [us],Sequence Id," << m_name << std::endl;
  outHeader.close ();
}



class NetFlowId
{
public:
  NetFlowId (uint32_t sonid, uint32_t soaid, uint32_t sinid, uint32_t siaid, uint16_t i = 0) 
    : sourceNodeId (sonid),
      sourceAppId (soaid),
      sinkNodeId (sinid),
      sinkAppId (siaid),
      index (i)
  {};
  friend bool operator== (NetFlowId f1, NetFlowId f2);
  uint32_t sourceNodeId;
  uint32_t sourceAppId;
  uint32_t sinkNodeId;
  uint32_t sinkAppId;
  uint16_t index;
};

inline bool 
operator== (NetFlowId f1, NetFlowId f2) 
{
  return (f1.sourceNodeId==f2.sourceNodeId) && (f1.sourceAppId==f2.sourceAppId) && (f1.sinkNodeId==f2.sinkNodeId) && (f1.sinkAppId==f2.sinkAppId);
}

class FlowData
{
public:
  FlowData (NetFlowId fid, std::string fn = "noname", bool singleFile=false);

  void PacketReceived (Ptr<const Packet> packet, bool singleFile);

  void SetFileName (std::string fileName) { m_fileName = fileName; };
  void SetFileNamePrefix (std::string fileNamePrefix) { m_fileNamePrefix = fileNamePrefix; };
  void SetFileWriteEnable (bool b) { m_fileWriteEnable = b; };
  bool IsFileWriteEnabled () { return m_fileWriteEnable; } ;
  void SetMemoryWriteEnable (bool b) { m_memoryWriteEnable = b; };
  bool IsMemoryWriteEnabled () { return m_memoryWriteEnable; };
  void Finalize (bool singleFile = false, uint32_t allRxPackets = 0); // Final calculations and write to file and to std::cout
  NetFlowId GetFlowId () { return m_flowId; };
private:
  NetFlowId m_flowId;
  std::string m_fileName;
  std::string m_fileNamePrefix;
  ScalarData m_scalarData;
  VectorData<Time> m_delayVector;
  bool m_fileWriteEnable;
  bool m_memoryWriteEnable;
};


class StatsFlows
{
public:
  StatsFlows (std::string fn = "noname") : m_fileName (fn), m_allRxPackets (0), m_singleFile (false) 
  { 
    if (m_fileName != "noname") 
      m_singleFile = true;
  };
  void PacketReceived (Ptr<const Packet> packet, uint32_t sinkNodeId, uint32_t sinkAppId);
  void Finalize ();

private:
  std::vector<NetFlowId> m_flowIds; 
  std::vector<FlowData> m_flowData;
  std::string m_fileName;
  uint32_t m_allRxPackets;
  bool m_singleFile;
};


} // namespace ns3

#endif // STATS_DATA_H
