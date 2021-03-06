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

#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"

#include "ns3/applications-module.h"

#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"

#include "custom-olsr-helper.h"


#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Project-2");

//std::string phyMode ("OfdmRate6Mbps");
std::string phyMode ("DsssRate1Mbps");
WifiPhyStandard phyStandard = WIFI_PHY_STANDARD_80211b;


// Configuration
std::string datarate     = "200kb/s";
uint32_t numNodes     	= 50;
double distance     	= 1500.0;         // distance between nodes, meters
int nSources     	    = 30;

bool enableCtsRts       = true;
double logDropOff         = 2.3;
bool useFriisDropoff     = false;
double mobilitySpeed     = 20.0;
double mobilityPause     = 0.001;

int seed                 = 4;
int mode                 = 0;

//Simulation Timing
float routingTime       = 0.0;          // time added to start for olsr to converge, seconds
double flowtime         = 10.0;           // total time each source will transmit for.
double sinkExtraTime    = 2.0;         // extra timer the last packet has to reach the sink, seconds

float totalTime         = routingTime + flowtime + sinkExtraTime; // total simulation time, seconds



bool outputFlowmon         = false;

bool enableNetanim         = false;
bool netanimCounters     = false;
bool packetMetadata        = true;


double counterInterval  = 0.5;            // netanim counter update interval, seconds


//Helpers
YansWifiPhyHelper wifiPhy;


//Objects
NodeContainer c;
NetDeviceContainer devices;
Ipv4InterfaceContainer i;

//Tracing
AnimationInterface *anim;
FlowMonitorHelper* flowmon;
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
    
    cmd.AddValue("seed", "seed", seed);
    cmd.AddValue("flows", "flows", nSources);
    cmd.AddValue("mobility", "mobility", mobilitySpeed);
    cmd.AddValue("mode", "mode", mode);
    
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
    
    //====================
    //PositionAllocator
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                  "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"),
                                  "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"));
    
    //=============
    //MobilityModel
    mobility.SetMobilityModel(
        "ns3::RandomWaypointMobilityModel",
        "Speed", PointerValue(CreateObjectWithAttributes<ns3::ConstantRandomVariable>("Constant", DoubleValue(mobilitySpeed))),
                              "Pause", PointerValue(CreateObjectWithAttributes<ns3::ConstantRandomVariable>("Constant", DoubleValue(mobilityPause))),
                              "PositionAllocator", PointerValue(CreateObjectWithAttributes<ns3::RandomRectanglePositionAllocator>(
                                  "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"),
                                                                                                                                  "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"))));
    
    
    mobility.Install(c);
    //Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange", MakeCallback(&CourseChange));
    
    //===========================
    // Enable OLSR
    CustomOlsrHelper olsr = CustomOlsrHelper();
    olsr.Set("Mode", UintegerValue(mode));
    
    Ipv4StaticRoutingHelper staticRouting;
    Ipv4ListRoutingHelper list;
    list.Add (olsr, 10);
    //list.Add (staticRouting, 0);
    
    //========================
    // Ipv4
    InternetStackHelper internet;
    internet.SetRoutingHelper(list);
    internet.Install (c);
    
    Ipv4AddressHelper ipv4;
    NS_LOG_INFO ("Assign IP Addresses.");
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    i = ipv4.Assign (devices);
    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
}

double RunUDPSourceSink()
{
    
    int port = 80;
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    
    ApplicationContainer app;
    
    for(int n = 0; n < nSources; n++) {
        int sourceNode = n;
        int sinkNode = numNodes - n - 1;
        
        OnOffHelper onoff = OnOffHelper("ns3::UdpSocketFactory", Address(InetSocketAddress(i.GetAddress(sinkNode), port)));
        onoff.SetConstantRate(DataRate(datarate));
        app = onoff.Install(c.Get(sourceNode));
        app.Start(Seconds(routingTime));
        app.Stop(Seconds(routingTime + flowtime));
        
        PacketSinkHelper sink = PacketSinkHelper("ns3::UdpSocketFactory", Address(InetSocketAddress(i.GetAddress(sinkNode), port)));
        app.Start(Seconds(routingTime + n * 0.1));
        app.Stop(Seconds(routingTime + flowtime + sinkExtraTime));
    }
    
    //====================
    //Simulation
    
    Simulator::Stop (Seconds(totalTime));
    Simulator::Run ();
    Simulator::Destroy ();
    
    //=====================
    //monitor dropped packets
    
    flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon->GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();
    NS_LOG_UNCOND("PACKETS: " << stats.size());
    
    int totalTxPackets = 0;
    int totalRxPackets = 0;
    int totalDropped = 0;
    double totalThroughput = 0.0;
    
    for(std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter) {
        
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
        
        double throughput = iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds() - iter->second.timeFirstTxPacket.GetSeconds()) / 1024;
        
        NS_LOG_UNCOND("Flow ID: " << iter->first << " Src_Addr: " << t.sourceAddress << " Dst_Addr: " << t.destinationAddress);
        NS_LOG_UNCOND("Tx Packets = " << iter->second.txPackets);
        NS_LOG_UNCOND("Rx PAckets = " << iter->second.rxPackets);
        NS_LOG_UNCOND("Dropped Packets = " << iter->second.txPackets - iter->second.rxPackets);
        NS_LOG_UNCOND("Throughput: " << throughput << " Kbps");
        
        totalTxPackets += iter->second.txPackets;
        totalRxPackets += iter->second.rxPackets;
        totalDropped += iter->second.txPackets - iter->second.rxPackets;
        totalThroughput += throughput;
        
    }
    
    //TRACE totals
    NS_LOG_UNCOND("\n====TOTALS====");
    NS_LOG_UNCOND("Flow ID: 0 -> " << nSources);
    NS_LOG_UNCOND("Tx Packets = " << totalTxPackets);
    NS_LOG_UNCOND("Rx PAckets = " << totalRxPackets);
    NS_LOG_UNCOND("Dropped Packets = " << totalDropped);
    NS_LOG_UNCOND("Throughput: " << totalThroughput << " Kbps");
    
    return totalThroughput;
}

int main(int argc, char *argv[])
{
    
    ParseCommands(argc, argv);
    
    ns3::SeedManager::SetSeed(seed);
    //ns3::SeedManager::SetRun(5);
    
    InitTopology();
    
    if(enableNetanim) {
        anim = new AnimationInterface("animation.xml");
        if(netanimCounters) {
            anim->EnableWifiPhyCounters(Seconds(0), Seconds(totalTime), Seconds(counterInterval));
            anim->EnableWifiMacCounters(Seconds(0), Seconds(totalTime), Seconds(counterInterval));
            anim->EnableIpv4L3ProtocolCounters(Seconds(0), Seconds(totalTime), Seconds(counterInterval));
        }
        if(packetMetadata) {
            anim->EnablePacketMetadata();
        }
        anim->SetStartTime(Seconds(0));
        anim->SetStopTime(Seconds(totalTime));
    }
    
    flowmon = new FlowMonitorHelper();
    flowMonitor = flowmon->InstallAll();  
    
    RunUDPSourceSink();
    
    if(enableNetanim) {
        NS_LOG_UNCOND ("Outputing NetAnim to animation.xml");
    }
    if(outputFlowmon) {
        NS_LOG_UNCOND ("Outputing FlowMonitor to flowmonitor.xml");
        flowMonitor->SerializeToXmlFile("flowmonitor.xml", true, true);
    }
    
    if(enableNetanim) delete anim;
    
    return 0;
}



//======TESTING Code=========//

double SingleExperiment(int s, int f, int ms, int md) {
    
    seed = s;
    nSources = f;
    mobilitySpeed = ms;
    mode = md;
    
    
    c = NodeContainer();
    devices = NetDeviceContainer();
    i = Ipv4InterfaceContainer();
    
    ns3::SeedManager::SetSeed(seed);
    //ns3::SeedManager::SetRun(5);
    
    
    NS_LOG_UNCOND("starting");
    std::clog << "s=" << s << ", f=" << f << ", ms=" << ms << ", md=" << md << std::endl;
    
    InitTopology();
    
    //FlowMonitorHelper flowmon;
    flowmon = new FlowMonitorHelper();
    flowMonitor = flowmon->InstallAll();
    
    
    double total =  RunUDPSourceSink();
    
    flowMonitor->Dispose();
    delete flowmon;
    
    return total;
}

//Used as a wrapper function around the project2.olsr.cc main method when
//doing large scale testing with various parameters
//Outputs a csv file which is fairly descriptive - each value is an average
int testmain (int argc, char *argv[]) //TODO remove the 1 to make this work
{
    int repeatedTrials     = 1;  //how many times each experiment is seeded and executed
    
    const int mobilitySt    = 1; //speed of nodes in m/s
    const int mobilityInc     = 5;  //distance between trials
    const int mobilityMax     = 21; //last value for trial
    
    const int nSourcesSt       = 5; //number of sources of flows
    const int nSourcesInc     = 5;  //distance between trials
    const int nSourcesMax     = 30; //last value for trial
    
    //Set up file for Output
    std::ofstream outputfile;
    outputfile.open ("results.csv");
    outputfile << "Results\n";
    
    //Changing Sources/flow count with constant mobility speed
    for (int mode = 0; mode < 5; mode++) {
        
        //Print heading
        outputfile << std::endl;
        if(mode == 0) outputfile << "Plain OLSR";
        else outputfile << "Modification " << mode; 
        outputfile << std::endl;
        
        //Flows
        outputfile << "flows (mobility=20),"; 
        for (int f = nSourcesSt; f <= nSourcesMax; f+=nSourcesInc) {
            outputfile << f << ",";
        }
        outputfile << std::endl;
        
        
        //Get results, each scenario on a new row;
        for (int seed = 1; seed <= repeatedTrials; seed++) {
            outputfile << "Throughput (seed=" << seed << "),";
            
            for (int f = nSourcesSt; f <= nSourcesMax; f+=nSourcesInc) {
                
                double result = 0;
                
                result = SingleExperiment(seed, f, 20, mode);
                outputfile << result << ",";
                
            }
            
            outputfile << std::endl;
        }
        outputfile << std::endl;
        
        //Mobility
        outputfile << "mobility (flows=20),";
        for (int m = mobilitySt; m <= mobilityMax; m += mobilityInc) {
            outputfile << m << ",";
        }
        outputfile << std::endl; 
        
        for (int seed = 1; seed <= repeatedTrials; seed++) {
            
            outputfile << "Throughput (seed=" << seed << "),";
            //reset number of Sources for the mobility tests
            for (int m = mobilitySt; m <= mobilityMax; m += mobilityInc) {
                double result = 0;
                
                result = SingleExperiment(seed, 20, m, mode);
                outputfile << result << ",";
            }
            
            outputfile << std::endl;
        }
        outputfile << std::endl;
    }
    outputfile.close();
    return 0;
}

