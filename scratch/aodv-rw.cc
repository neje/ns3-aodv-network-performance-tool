/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 The Boeing Company
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
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/propagation-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("AodvRw");

uint64_t countAodv = 0, countRreq = 0, countRrep = 0, countRerr = 0, countRrepAck = 0;
Time firstAodv, lastAodv;
uint64_t bytesAodv = 0;
std::string overheadFileName;
  
// Function for capturing AODV overhead statistics
void
AodvPacketTrace (std::string context, Ptr<const Packet> packet)
{
  std::string packetType;
  std::ostringstream description;
  aodv::TypeHeader tHeader;
  Ptr<Packet> p = packet->Copy ();
  p->RemoveHeader (tHeader);
  if (!tHeader.IsValid ())
    {
      NS_LOG_DEBUG ("AODV message " << packet->GetUid () << " with unknown type received: " << tHeader.Get () << ". Drop");
      return; // drop
    }
  switch (tHeader.Get ())
    {
    case aodv::MessageType::AODVTYPE_RREQ:
      {
        packetType = "RREQ";
        aodv::RreqHeader rreqHeader;
        p->RemoveHeader (rreqHeader);
        description << "O:" << rreqHeader.GetOrigin () << " D:" << rreqHeader.GetDst () << " Hop:" << (int)rreqHeader.GetHopCount (); 
        countRreq++;
        break;
      }
    case aodv::MessageType::AODVTYPE_RREP:
      {
        packetType = "RREP";
        aodv::RrepHeader rrepHeader;
        p->RemoveHeader (rrepHeader);
        description << "O:" << rrepHeader.GetOrigin () << " D:" << rrepHeader.GetDst () << " Hop:" << (int)rrepHeader.GetHopCount (); 
        countRrep++;
        break;
      }
    case aodv::MessageType::AODVTYPE_RERR:
      {
        packetType = "RERR";
        countRerr++;
        break;
      }
    case aodv::MessageType::AODVTYPE_RREP_ACK:
      {
        packetType = "RREP_ACK";
        countRrepAck++;
        break;
      }
    }

  countAodv++;
  if (countAodv==1)
  {
    firstAodv = Simulator::Now ();
  }
  lastAodv = Simulator::Now ();
  uint64_t packetSize = packet->GetSize ();
  bytesAodv += packetSize;

  // Write packet data to file
  std::ofstream out (overheadFileName.c_str (), std::ios::app);
  // Time [us], Packet Type, Length [B], Description, Context
  out << lastAodv.GetDouble () / 1000.0 << ","
      << packetType << ","
      << packetSize << ","
      << description.str () << "," // currently Description field is used for RREQ and RREP only
      << context << std::endl;
  out.close ();
  NS_LOG_INFO ("AODV stats: " << lastAodv << ", #" << countAodv << ", bytes: " << bytesAodv);
}

// Function for periodical write of the current time on the screan
void
SimulationRunTime ()
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds ());
  Simulator::Schedule (Seconds (1.0), &SimulationRunTime);
}

// MAIN function
int
main (int argc, char *argv[])
{
  //LogComponentEnable("StatsData", LOG_LEVEL_ALL);
  //LogComponentEnable("AodvRw", LOG_LEVEL_ALL);
  
  std::string csvFileNamePrefix = "aodv-rw";
  std::string phyMode ("DsssRate5_5Mbps");
  bool verbose = false;
  uint32_t nNodes = 60; // Number of nodes
  std::string protocol = "ns3::UdpSocketFactory";
  uint16_t port = 80;
  std::string dataRateStr = "50kbps"; // 20kbps, 50kbps, 100kbps
  uint32_t packetSize = 512; // Bytes
  uint32_t simulationDuration = 200; // Seconds
  uint32_t startupTime = 10; // Seconds
  uint32_t nActiveNodes = 12; // Nodes that generate traffic (max = nNodes/2); 3, 6, 12 active nodes 
  double nodeSpeed = 10; // 1.5, 5, 10, 15, 20, 25 [m/s]
  uint32_t areaSide = 500; // [m] square area

  uint8_t appStartDistance = 0; // [s] time that shuld be enough to find the route and stop sendnig new RREQ packets

  CommandLine cmd;
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  //cmd.AddValue ("transport TypeId", "TypeId for socket factory", protocol);
  cmd.AddValue ("dataRate", "Application data rate", dataRateStr);
  cmd.AddValue ("packetSize", "Size of application packets in bytes", packetSize);
  //cmd.AddValue ("csvFileNamePrefix", "First part of file name: csvFileNamePrefix-dataRate-packetSize.csv", csvFileNamePrefix);
  //cmd.AddValue ("simulationDuration", "Duration of simulation", simulationDuration);
  cmd.AddValue ("nNodes", "Number of nodes in simulation", nNodes);
  cmd.AddValue ("nActiveNodes", "Number of nodes that send data (max = nNodes/2)", nActiveNodes);
  cmd.AddValue ("areaSide", "Side of square simulation area in meters", areaSide);
  cmd.AddValue ("nodeSpeed", "Constant speed of nodes in Gaus-Marcov model", nodeSpeed);
  cmd.AddValue ("appStartDistance", "Time between application start (that shuld be enough to find the route and stop sendnig new RREQ packets)", appStartDistance);
  cmd.Parse (argc, argv);

  // File names. Changes: nActiveNodes, dataRate
  csvFileNamePrefix += "-" + std::to_string (areaSide) + "mx" + std::to_string (areaSide) + "m"
                    + "-nodes" + std::to_string (nActiveNodes) + "_" + std::to_string (nNodes)
                    + "-" + dataRateStr  
                    + "-speed" + std::to_string (nodeSpeed) 
                    + "-" + std::to_string (packetSize) + "B";
  std::string flowFileName = csvFileNamePrefix + "-flow.csv";
  overheadFileName = csvFileNamePrefix + "-overhead.csv";
  
  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

  NodeContainer c;
  c.Create (nNodes);

  // Mobility of the nodes
  MobilityHelper mobility;

  std::string speed = "ns3::ConstantRandomVariable[Constant=" + std::to_string (nodeSpeed)  + "]";
  std::string randomVariable = "ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string (areaSide) + "]";

  Ptr<RandomRectanglePositionAllocator> positionAlloc = CreateObject<RandomRectanglePositionAllocator> ();
  positionAlloc->SetAttribute ("X", StringValue (randomVariable.c_str ()));
  positionAlloc->SetAttribute ("Y", StringValue (randomVariable.c_str ()));

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
           "Speed", StringValue(speed.c_str ()), //1.5, 5.0, 10.0, 15.0, 20.0, 25.0
           "Pause", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=10.0]"),
           "PositionAllocator", PointerValue (positionAlloc));
  mobility.Install (c);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  if (verbose)
    {
      wifi.EnableLogComponents ();  // Turn on all Wifi logging
      LogComponentEnable ("AodvRoutingProtocol", LOG_LEVEL_DEBUG); // AODV logging
    }
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel",
                                  "Exponent", DoubleValue (2.8),
                                  "ReferenceDistance", DoubleValue (1.0),
                                  "ReferenceLoss", DoubleValue (40.046));
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);

  InternetStackHelper internet;
  // AODV - set routing protocol !!!
  AodvHelper aodv;
  aodv.Set ("EnableHello", BooleanValue (false));
  internet.SetRoutingHelper(aodv);
  internet.Install (c);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  // Applications
  TypeId tid = TypeId::LookupByName (protocol); // Set transport layer protocol TCP or UDP
  DataRate dataRate = DataRate (dataRateStr); // Application data rate

  for (uint32_t i = 0; i<nActiveNodes; i++)
  {
    std::ostringstream oss;
    oss <<  "192.168.1." << i+1;
    InetSocketAddress destinationAddress = InetSocketAddress (Ipv4Address (oss.str().c_str ()), port);
    InetSocketAddress sinkAddress = InetSocketAddress (Ipv4Address::GetAny (), port);
   
    // Source
    StatsSourceHelper sourceAppH (protocol, destinationAddress);
    sourceAppH.SetConstantRate (dataRate);
    sourceAppH.SetAttribute ("PacketSize", UintegerValue(packetSize));
    ApplicationContainer sourceApps = sourceAppH.Install (c.Get (nNodes-1-i));
    sourceApps.Start (Seconds (startupTime+i*appStartDistance)); // Every app starts "appStartDistance" seconds after previous one
    sourceApps.Stop (Seconds (startupTime+i*appStartDistance+simulationDuration)); // Every app stops after finishes runnig of "simulationDuration" seconds
   
    // Sink 
    StatsSinkHelper sink (protocol, sinkAddress);
    ApplicationContainer sinkApps = sink.Install (c.Get (i));
    sinkApps.Start (Seconds (startupTime+i*appStartDistance-1)); // start before source to prepare for first packet
    sinkApps.Stop (Seconds (startupTime+simulationDuration+i*appStartDistance+1)); // stop a bit later then source to receive the last packet
  }
 
  // Tracing
  Config::Connect ("/NodeList/*/$ns3::aodv::RoutingProtocol/Tx", MakeCallback (&AodvPacketTrace));
  StatsFlows sf (flowFileName.c_str ());
  Config::ConnectWithoutContext ("/NodeList/*/ApplicationList/*/$ns3::StatsPacketSink/Rx", MakeCallback (&StatsFlows::PacketReceived, &sf));
  
  // Event for periodical write of the current time on the screan
  Simulator::Schedule (Seconds (0.0), &SimulationRunTime);

  //Overhead statistics
  std::ofstream outh (overheadFileName.c_str ());
  outh << "Time [us], Packet Type, Length [B], Description, Context" << std::endl;
  outh.close ();

  // Start-stop simulation
  // Stop event is set so that all applications have enough tie to finish 
  Simulator::Stop (Seconds (startupTime+(nNodes-1)*appStartDistance+simulationDuration+1));
  Simulator::Run ();

  // Write filnal statistics
  // Flow statistics
  sf.Finalize ();
  
  //Overhead statistics
  std::ofstream outf (overheadFileName.c_str (), std::ios::app);
  outf << std::endl;
  outf << "AODV overhead [packets]:," << countAodv << std::endl;
  outf << "AODV overhead [kB]:," << (double)bytesAodv/1000.0 << std::endl;
  outf << "RREQ [packets]:," << countRreq << std::endl;
  outf << "RREP [packets]:," << countRrep << std::endl;
  outf << "RERR [packets]:," << countRerr << std::endl;
  outf << "RREP_ACK [packets]:," << countRrepAck << std::endl;
  outf.close ();

  // End of simulation
  Simulator::Destroy ();

  return 0;
}
