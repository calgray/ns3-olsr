/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
* Copyright (c) 2009 University of Washington
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

// UWA 2016 CITS4419 Project - Callan Gray (21341958)
// Based on the provided ns3 sample in directory examples/wireless/wifi-simple-adhoc-grid.cc

//Nodes: 50 in area of 1500x1500m
//3 mobility speeds: 1m/s, 10m/s, 20m/s 
//Mobility: random way point mobility
//CBR traffic sources: node 10, 20, 30
//performance => throughput of protocol

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"

#include "ns3/applications-module.h"

#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Project-2");

//std::string phyMode ("OfdmRate6Mbps");
std::string phyMode ("DsssRate1Mbps");
WifiPhyStandard phyStandard = WIFI_PHY_STANDARD_80211b;


// Configuration
std::string datarate 	= "10kb/s";
uint32_t numNodes       = 50;
double distance         = 1500.0;         // distance between nodes, meters
bool enableCtsRts       = true;
bool useFriisDropoff 	= false;

double logDropOff 	= 2.5;
double mobilitySpeed	= 20.0;

//Simulation Timing
float routingTime       = 0.0;          // time added to start for olsr to converge, seconds
float simTime           = 20.0;		 // total simulation time after routing, seconds 
double flowtime     	= 8.0;           // total time each source will transmit for.
double sinkExtraTime    = 2.0;		 // extra timer the last packet has to reach the sink, seconds

bool synchronisedStop   = false;         // whether source 2 and 3 should stop at the same time as source 1

bool netanimCounters 	= false;
double counterInterval  = 0.5;            // netanim counter update interval, seconds


uint32_t sourceNode1 	= 3;
uint32_t sinkNode1 	= 49;

uint32_t sourceNode2 	= 42;
uint32_t sinkNode2 	= 25;

uint32_t sourceNode3 	= 12;
uint32_t sinkNode3 	= 1;



//Helpers
YansWifiPhyHelper wifiPhy;
OlsrHelper olsr;
Ipv4StaticRoutingHelper staticRouting;

//Objects
NodeContainer c;
NetDeviceContainer devices;
Ipv4InterfaceContainer i;

//Tracing
AnimationInterface *anim;
FlowMonitorHelper flowmon;
Ptr<FlowMonitor> flowMonitor;

bool verbose = false;
bool tracing = false;

void ParseCommands(int argc, char **argv)
{
  CommandLine cmd;

  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("distance", "distance (m)", distance);
  cmd.AddValue ("enableCtsRts", "enable CTS/RTS", enableCtsRts);
  cmd.AddValue ("tracing", "turn on ascii and pcap tracing", tracing);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  
  cmd.Parse (argc, argv);
}


//static in this context means make all variables in here persistant
//static void CourseChange(std::string foo, Ptr<const MobilityModel> mobility) {
  //Vector pos = mobility->GetPosition();
  //NS_LOG_UNCOND(pos);
//}

void InitTopology()
{
  //============================================
  //Extra settings
  
  // disable fragmentation for frames below 3000 bytes
  //Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("3000"));
  
  // turn on/off RTS/CTS for frames below 3000 bytes
  UintegerValue ctsThr = (enableCtsRts ? UintegerValue (100) : UintegerValue (5000));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", ctsThr);
  
  // Fix non-unicast data rate to be the same as that of unicast
  //Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));

  c.Create (numNodes);
  
  //=====================
  //wifi - standard/modes
  WifiHelper wifi;
  if(verbose) { wifi.EnableLogComponents (); } // Turn on all Wifi logging
  wifi.SetStandard (phyStandard);  
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
				"DataMode",StringValue (phyMode),
				"ControlMode",StringValue (phyMode));
  
  //==============================
  //wifi - MAC
  // Add an upper mac and disable rate control
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  
  //===============================
  //wifi - Phy
  wifiPhy = YansWifiPhyHelper::Default ();  
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  if(tracing) wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 

  //====================================
  //wifi - Channel
  //YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
  
  YansWifiChannelHelper wifiChannel;
  
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  if(useFriisDropoff) {
    wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  }
  else {
    wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel", "Exponent", DoubleValue(logDropOff));
  }
  
  wifiPhy.SetChannel (wifiChannel.Create ());
  
  
  //=======================
  //wifi - Install
  devices = wifi.Install (wifiPhy, wifiMac, c);

  if(tracing) {
    wifiPhy.EnablePcap("wifi-adhoc", devices);
    wifiPhy.EnableAscii("wifi-adhoc", devices);
  }
  
  //========================
  //mobility - Grid Allocator
  MobilityHelper mobility;
  //GridPositionAllocator
  
  //====================
  //PositionAllocator
  
  /*
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
				 "MinX", DoubleValue (0.0),
				 "MinY", DoubleValue (0.0),
				 "DeltaX", DoubleValue (distance/7),
				 "DeltaY", DoubleValue (distance/7),
				 "GridWidth", UintegerValue (7),
				 "LayoutType", StringValue ("RowFirst"));
  */
  
  /*
  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
				 "X", StringValue("100.0"),
				 "Y", StringValue("100.0"),
				 "Rho", StringValue("ns3::UniformRandomVariable[Min=0,Max=20.0]"));
 */
  
  
  mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
				"X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"),
				"Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"));

  
  
  //=============
  //MobilityModel

    
  //mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  
  

  /*
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue("0.1s"), //move interval
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=10.0]"),
                             "Bounds", StringValue ("0|1500|0|1500"));
  */
  
  
  mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
			    "Speed", PointerValue(CreateObjectWithAttributes<ns3::ConstantRandomVariable>("Constant", DoubleValue(mobilitySpeed))), 
			    "Pause", PointerValue(CreateObjectWithAttributes<ns3::ConstantRandomVariable>("Constant", DoubleValue(0.1))),
			    "PositionAllocator", PointerValue(CreateObjectWithAttributes<ns3::RandomRectanglePositionAllocator>(
			    "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"),
			    "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"))));
  
 
  mobility.Install(c);
  
  //Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange", MakeCallback(&CourseChange));
  
  //===========================
  // Enable OLSR
  Ipv4ListRoutingHelper list;
  list.Add (olsr, 10);
  list.Add (staticRouting, 0);

  
  
  

  //========================
  // Ipv4
  InternetStackHelper internet;
  internet.SetRoutingHelper(list); // has effect on the next Install ()
  internet.Install (c);
  
  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  i = ipv4.Assign (devices);
  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  
}

void ReceivePacket (Ptr<Socket> socket)
{
  while (Ptr<Packet> packet = socket->Recv ())
    {
	//NS_LOG_UNCOND ("Received one packet!");
    }
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize, uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
    {
      socket->Send (Create<Packet> (pktSize));
      Simulator::Schedule (pktInterval, &GenerateTraffic, socket, pktSize, pktCount-1, pktInterval);
    }
  else
    {
      socket->Close ();
    }
}

void RunUDPSourceSink()
{
  int port = 80;
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  float startTime1 = 1.0;
  float startTime2 = 1.5;
  float startTime3 = 2.0;
  
  ApplicationContainer app;
  
  //=========================
  //Flow 1 : 0 -> 24
  
  OnOffHelper onoff = OnOffHelper("ns3::UdpSocketFactory", Address(InetSocketAddress(i.GetAddress(sinkNode1), port)));
  onoff.SetConstantRate(DataRate(datarate));
  app = onoff.Install(c.Get(sourceNode1));
  app.Start(Seconds(routingTime + startTime1));
  app.Stop(Seconds(routingTime + startTime1 + flowtime));
  
  PacketSinkHelper sink = PacketSinkHelper("ns3::UdpSocketFactory", Address(InetSocketAddress(i.GetAddress(sinkNode1), port)));
  app = sink.Install(c.Get(sinkNode1));
  app.Start(Seconds(routingTime + startTime1));
  app.Stop(Seconds(routingTime + startTime1 + flowtime + sinkExtraTime));

  
  //=========================
  //Flow 2 : 10 -> 14
  
  onoff = OnOffHelper("ns3::UdpSocketFactory", Address(InetSocketAddress(i.GetAddress(sinkNode2), port)));
  onoff.SetConstantRate(DataRate(datarate));
  app = onoff.Install(c.Get(sourceNode2));
  app.Start(Seconds(routingTime + startTime2));
  app.Stop(Seconds(routingTime + startTime2 + flowtime));
  if(synchronisedStop) {
    app.Stop(Seconds(routingTime + startTime1 + flowtime));
  } else {
    app.Stop(Seconds(routingTime + startTime2 + flowtime));
  }
  
  sink = PacketSinkHelper("ns3::UdpSocketFactory", Address(InetSocketAddress(i.GetAddress(sinkNode2), port)));
  app = sink.Install(c.Get(sinkNode2));
  app.Start(Seconds(routingTime + startTime2));
  app.Stop(Seconds(routingTime + startTime2 + flowtime + sinkExtraTime));
  
  //=======================
  //Flow 3 : 20 -> 4
  
  onoff = OnOffHelper("ns3::UdpSocketFactory", Address(InetSocketAddress(i.GetAddress(sinkNode3), port)));
  onoff.SetConstantRate(DataRate(datarate));
  app = onoff.Install(c.Get(sourceNode3));
  app.Start(Seconds(routingTime + startTime3));
    if(synchronisedStop) {
    app.Stop(Seconds(routingTime + startTime1 + flowtime));
  } else {
    app.Stop(Seconds(routingTime + startTime3 + flowtime));
  }
  
  sink = PacketSinkHelper("ns3::UdpSocketFactory", Address(InetSocketAddress(i.GetAddress(sinkNode3), port)));
  app = sink.Install(c.Get(sinkNode3));
  app.Start(Seconds(routingTime + startTime3));
  app.Stop(Seconds(routingTime + startTime3 + flowtime + sinkExtraTime));

  //====================
  //these variables get removed after running
  Ipv4Address src1 = i.GetAddress(sourceNode1);
  Ipv4Address snk1 = i.GetAddress(sinkNode1);
  Ipv4Address src2 = i.GetAddress(sourceNode2);
  Ipv4Address snk2 = i.GetAddress(sinkNode2);
  Ipv4Address src3 = i.GetAddress(sourceNode3);
  Ipv4Address snk3 = i.GetAddress(sinkNode3);
  
  //====================
  //Simulation
  
  Simulator::Stop (Seconds (routingTime + simTime));
  Simulator::Run ();
  Simulator::Destroy ();
  
  //=====================
  //monitor dropped packets
  
  flowMonitor->CheckForLostPackets();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
  std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();
  NS_LOG_UNCOND("PACKETS: " << stats.size());
  
  for(std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter) {
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
    
    //TODO: need to retreive the ip address from the container
    if(  (t.sourceAddress == src1  && t.destinationAddress == snk1)
      || (t.sourceAddress == src2  && t.destinationAddress == snk2)
      || (t.sourceAddress == src3  && t.destinationAddress == snk3))
    {    
      NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr: " << t.sourceAddress << " Dst Addr: " << t.destinationAddress);
      NS_LOG_UNCOND("Tx Packets = " << iter->second.txPackets);
      NS_LOG_UNCOND("Rx PAckets = " << iter->second.rxPackets);
      NS_LOG_UNCOND("Dropped Packets = " << iter->second.txPackets - iter->second.rxPackets);
      
      NS_LOG_UNCOND(
	"Throughput: " 
	<< iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds() 
	- iter->second.timeFirstTxPacket.GetSeconds()) / 1024 
	<< " Kbps");
    }
  }
}

int main (int argc, char *argv[])
{

  ParseCommands(argc, argv);

  ns3::SeedManager::SetSeed(3);
  //ns3::SeedManager::SetRun(5);
  
  InitTopology();

  //NetAnim Setup
  NS_LOG_UNCOND ("Outputing NetAnim to animation.xml");
  anim = new AnimationInterface("animation.xml");
  if(netanimCounters) {
    anim->EnableWifiPhyCounters(Seconds(routingTime), Seconds(routingTime + simTime), Seconds(counterInterval));
    anim->EnableWifiMacCounters(Seconds(routingTime), Seconds(routingTime + simTime), Seconds(counterInterval));
    anim->EnableIpv4L3ProtocolCounters(Seconds(routingTime), Seconds(routingTime + simTime), Seconds(counterInterval));
    anim->EnablePacketMetadata();
  }
  anim->SetStartTime(Seconds(routingTime));
  anim->SetStopTime(Seconds(routingTime + simTime));

  
  

  flowMonitor = flowmon.InstallAll();
  
  RunUDPSourceSink();
  
  NS_LOG_UNCOND ("Outputing FlowMonitor to flowmonitor.xml");
  flowMonitor->SerializeToXmlFile("flowmonitor.xml", true, true);

  
  delete anim;
  
  return 0;
}

