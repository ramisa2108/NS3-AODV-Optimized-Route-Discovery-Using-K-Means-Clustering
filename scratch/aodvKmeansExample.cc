/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
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
 * This is an example script for AODV manet routing protocol. 
 *
 * Authors: Pavel Boyko <boyko@iitp.ru>
 */

#include<iostream>
#include<fstream>
#include "ns3/core-module.h"
#include "ns3/aodv-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/aodvKmeans-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AODV-Kmeans-Example");

class AODVExample
{

public:
    AODVExample();
    // run this experiment
    void Run();
    // configure script parameters
    void CommandSetUp(int argc, char * argv[]);
    // print arguments
    void PrintArguments();
    // print node positions
    void PrintPositions();

private:

// parameters

    // number of nodes in the network
    int m_nodes;
    // node movement speed in m/s
    int m_speed;
    // node pause time in s
    int m_pause;
    // total simulation time
    double m_totalTime;
    // total bytes transmitted
    uint32_t m_totalBytes;
    // total packets received 
    uint32_t m_totalPackets;
    // number of flows
    int m_nflows;
    // total transmission power
    double m_txp;
    // trace mobility if true
    bool m_traceMobility;
    // print routies if true
    bool m_printRoutes;
    // move clients away after 1/2 of simulation
    bool m_moveClients;
    
    // total sent packets
    int m_packetsSent;
    // total received packets
    int m_packetsReceived;
    // total dropped packets 
    int m_packetsDropped;


    // nodes used in this example
    NodeContainer nodes;
    // netdevices used in this example
    NetDeviceContainer devices;
    // interfaces used in this example
    Ipv4InterfaceContainer interfaces;

    // client nodes
    std::vector<int>m_clients;
    // server nodes
    std::vector<int>m_servers;

    // prefix for filenames
    std::string m_prefix;
    // int packets per second
    int m_packets_per_second;
    // intervals
    double m_interval;
    // protocol 1: krop, 2: aodv
    int m_protocol;


    
   

// functions
    // create the nodes
    void CreateNodes();
    // create the devices
    void CreateDevices();
    // create network
    void CreateInternetStacks();
    // create application
    void InstallApplications();
    
    
    // callback function for packet send
    void SendPkt (Ptr< const Packet > packet, Ptr< Ipv4 > ipv4, uint32_t interface);
    // callback function for packet send
    void ReceivePkt (Ptr< const Packet > packet, Ptr< Ipv4 > ipv4, uint32_t interface);


    // check if the address is a servr
    bool MatchServerClient(Ipv4Address address, Ipv4Address address2);

    void ReceivePacket (Ptr<Socket> socket);
    

};


AODVExample::AODVExample():
    m_nodes(15),
    m_speed(20),
    m_pause(0),
    m_totalTime(10),
    m_totalBytes(0),
    m_totalPackets(0),
    m_nflows(6),
    m_txp(7.5),
    m_traceMobility(false),
    m_printRoutes(false),
    m_moveClients(false),
    m_packetsSent(0),
    m_packetsReceived(0),
    m_packetsDropped(0),
    m_prefix("project-baseline"),
    m_packets_per_second(1),
    m_protocol(1)
    
{
   
}
void
AODVExample::CommandSetUp(int argc, char *argv[])
{
    CommandLine cmd(__FILE__);
    cmd.AddValue("nodes", "Total number of nodes in the network", m_nodes);
    cmd.AddValue("speed", "Speed of nodes moving", m_speed);
    cmd.AddValue("pause", "Pause time for nodes movement", m_pause);
    cmd.AddValue("time", "Total time the simulation will run", m_totalTime);
    cmd.AddValue("flows", "Number of flows", m_nflows);
    cmd.AddValue("txp", "Transmission power", m_txp);
    cmd.AddValue("traceMobility", "Enable mobility tracing", m_traceMobility);
    cmd.AddValue("printRoute", "Enable printing routes", m_printRoutes);
    cmd.AddValue("moveclient", "Move clients away after some time", m_moveClients);
    cmd.AddValue("prefix", "Prefix of all generated file names\n", m_prefix);
    cmd.AddValue("packetsPerSecond", "Packets Sent per second", m_packets_per_second);
    cmd.AddValue("protocol", "Protocol to use 1: Krop 2: AODV", m_protocol);
    
    cmd.Parse (argc, argv);
    return;
}

void
AODVExample::PrintArguments()
{
    std::cout << "Simulation arguments:\n";
    std::cout << "nodes: " << m_nodes << "\n";
    std::cout << "flows: " << m_nflows << "\n";
    std::cout << "speed: " << m_speed << "\n";
    std::cout << "time: " << m_totalTime << "\n";
    std::cout << "pps: " << m_packets_per_second << "\n";
    std::cout << "--------------------------\n";
}

void 
AODVExample::PrintPositions()
{
    std::cout << "Time: " << Simulator::Now().GetSeconds() << "\n";
    Simulator::Schedule(Seconds(1.0), &AODVExample::PrintPositions, this);

}

void 
AODVExample::Run()
{

    PrintArguments();

    
    Packet::EnablePrinting();

    
    //Set Non-unicastMode rate to unicast mode
    Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue ("DsssRate11Mbps"));

    std::cout << "Setting networks...\n";
    CreateNodes();
    CreateDevices();
    CreateInternetStacks();
    InstallApplications();

    
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

    NS_LOG_INFO("Running simulations...");
    
    Simulator::Stop(Seconds(m_totalTime));

    std::ofstream positionPrint("project-baseline-node-positions.txt");
    positionPrint.close();
    Simulator::Schedule(Seconds(1.0), &AODVExample::PrintPositions, this);

    
    
    // setup animation
    // AnimationInterface anim (m_prefix + "-animation.xml");
    // anim.SetMaxPktsPerTraceFile(1000000);
    
    

    Simulator::Run();

    // this must be added after simulator::run()  
    
    int j=0;
    float AvgThroughput = 0;
    Time Jitter;
    Time Delay;

    // variables for output measurement
    uint32_t SentPackets = 0;
    uint32_t ReceivedPackets = 0;
    uint32_t DroppedPackets = 0;

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();

    
    std::ofstream flowOut("TaskB/"+ m_prefix+"-flowstats");
    bool printConsole = true;

    for (auto iter = stats.begin (); iter != stats.end (); ++iter) {
            SentPackets = SentPackets +(iter->second.txPackets);
            ReceivedPackets = ReceivedPackets + (iter->second.rxPackets);
            DroppedPackets += iter->second.txPackets - iter->second.rxPackets;
            AvgThroughput = AvgThroughput + iter->second.rxBytes;
            
            Delay = Delay + (iter->second.delaySum);
            
            
            j = j + 1;

        

    }

    //AvgThroughput = AvgThroughput/j;
    AvgThroughput = AvgThroughput * 8.0 / ((m_totalTime-1.0) * 1024.0);
    Delay = Delay / ReceivedPackets;
    
    flowOut << "--------Total Results of the simulation----------"<<std::endl << "\n";
    flowOut << "Nodes: " << m_nodes << "\n";
    flowOut << "Flows: " << m_nflows << "\n";
    flowOut << "pps: " << m_packets_per_second << "\n";
    flowOut << "speed: " << m_speed << "\n";
    flowOut << "Total Received Packets = " << ReceivedPackets << "\n";
    flowOut << "Total sent packets  = " << SentPackets << "\n";
    flowOut << "Total Lost Packets = " << DroppedPackets << "\n";
    flowOut << "Packet Loss ratio = " << ((DroppedPackets*100.00)/SentPackets)<< " %" << "\n";
    flowOut << "Packet delivery ratio = " << ((ReceivedPackets*100.00)/SentPackets)<< " %" << "\n";
    flowOut << "Average Throughput = " << AvgThroughput<< " Kbps" << "\n";
    flowOut << "End to End Delay = " << Delay << "\n";
    flowOut << "Total Flow id " << j << "\n";

    if(printConsole)
    {
        std::cout << "\n--------Total Results of the simulation from flow monitor----------\n";
        std::cout << "Total Flow id " << j << "\n";
        std::cout << "Total sent packets  = " << SentPackets << "\n";
        std::cout << "Total Received Packets = " << ReceivedPackets << "\n";
        std::cout << "End to End Delay = " << Delay << "\n";
        std::cout << "Packet delivery ratio = " << ReceivedPackets << " / " << SentPackets << " = " << ((ReceivedPackets*100.00)/SentPackets)<< "%" << "\n";
        std::cout << "Packet Loss ratio = " << ((DroppedPackets*100.00)/SentPackets)<< " %" << "\n";
        std::cout << "Average Throughput = " << AvgThroughput<< "Kbps" << "\n\n";
    }


    flowOut.close();
    Simulator::Destroy();

    
    

}

void 
AODVExample::CreateNodes()
{

    nodes.Create(m_nodes);
    
    // Name the nodes
    for(int j=0;j<m_nodes;j++){
        std::ostringstream os;
        os << "node-" << j;
        Names::Add(os.str(), nodes.Get(j));
    }

    // create mobility model
    MobilityHelper mobilityHelper;
    int64_t streamIndex = 0; // used to get consistent mobility across scenarios

    ObjectFactory pos;
    pos.SetTypeId("ns3::RandomRectanglePositionAllocator");
    pos.Set("X", StringValue("ns3::UniformRandomVariable[Min=0|Max=300]"));
    pos.Set("Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=300]"));

    Ptr<PositionAllocator> positionAlloc = pos.Create()->GetObject<PositionAllocator>();
    streamIndex += positionAlloc->AssignStreams(streamIndex);

    std::stringstream ssSpeed;
    ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << m_speed << "]";
    std::stringstream ssPause;
    ssPause << "ns3::ConstantRandomVariable[Constant=" << m_pause << "]";

    mobilityHelper.SetMobilityModel(
        "ns3::RandomWaypointMobilityModel",
        "Speed", StringValue(ssSpeed.str()),
        "Pause", StringValue(ssPause.str()),
        "PositionAllocator", PointerValue(positionAlloc)
    );
    mobilityHelper.SetPositionAllocator(positionAlloc);
    mobilityHelper.Install(nodes);
    streamIndex += mobilityHelper.AssignStreams(nodes, streamIndex);

    
    NS_UNUSED(streamIndex);

    
    std::cout << "nodes created\n";

    
}
void 
AODVExample::CreateDevices()
{

    // setting up wifi phy and channel using helpers
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);

    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");
    wifiPhy.SetChannel(wifiChannel.Create());

    // Add a mac and disable rate control
    WifiMacHelper wifiMac;
    wifi.SetRemoteStationManager(
        "ns3::ConstantRateWifiManager",
        "DataMode", StringValue("DsssRate11Mbps"),
        "ControlMode", StringValue("DsssRate11Mbps")
    );

    wifiPhy.Set("TxPowerStart", DoubleValue(m_txp));
    wifiPhy.Set("TxPowerEnd", DoubleValue(m_txp));

    wifiMac.SetType("ns3::AdhocWifiMac");
    devices = wifi.Install(wifiPhy, wifiMac, nodes);

            

    AsciiTraceHelper ascii;
    wifiPhy.EnableAsciiAll(ascii.CreateFileStream(m_prefix + "-phy.tr"));
  
    std::cout << "devices created\n"; 



}
void 
AODVExample::CreateInternetStacks()
{

    InternetStackHelper internet;
    AodvHelper aodv;
    KropHelper krop;
    if(m_protocol == 1)
    {
        internet.SetRoutingHelper(krop);
    }
    else 
    {
        internet.SetRoutingHelper(aodv);
    }
    //
    
    internet.Install(nodes);
    NS_LOG_INFO ("assigning ip address");

    Ipv4AddressHelper addressHelper;
    
    addressHelper.SetBase ("10.1.1.0", "255.255.255.0");

    interfaces = addressHelper.Assign(devices);

    
    if (m_printRoutes)
    {
        Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (m_prefix + ".routes", std::ios::out);
        if(m_protocol == 1)
            krop.PrintRoutingTableAllAt (Seconds (m_totalTime), routingStream);
        else
            aodv.PrintRoutingTableAllAt (Seconds (m_totalTime), routingStream);
        
    }

    AsciiTraceHelper ascii;
    internet.EnableAsciiIpv4All(ascii.CreateFileStream(m_prefix + "-ipv4.tr"));
    


}

void 
AODVExample::InstallApplications()
{
    UdpEchoServerHelper echoServer(9);


    
    m_clients.reserve(m_nflows / 2);
    m_servers.reserve(m_nflows / 2);
    std::ofstream serverClient("TaskB/"+ m_prefix + "-server-client.txt");
    
    for(int i=0;i<m_nflows/2;i++)
    {
        m_clients[i] = m_nodes-i*2-1;
        m_servers[i] = i*2;
        serverClient << interfaces.GetAddress(m_servers[i]) << "\n";
        serverClient << interfaces.GetAddress(m_clients[i]) << "\n";
        
    }

    serverClient.close();

    m_interval = 1.0 / m_packets_per_second;
    
    std::cout << "Setting up flows " << std::endl;
    for(int i=0; i<m_nflows/2; i++){

        ApplicationContainer serverApps = echoServer.Install(nodes.Get(m_servers[i]));
        serverApps.Start(Seconds(0.1));
        serverApps.Stop(Seconds(m_totalTime));

        UdpEchoClientHelper echoClient(interfaces.GetAddress(m_servers[i]), 9);
        echoClient.SetAttribute("MaxPackets", UintegerValue(10000));
        echoClient.SetAttribute("Interval", TimeValue(Seconds(m_interval)));
        echoClient.SetAttribute("PacketSize", UintegerValue(64));



        ApplicationContainer clientApps = echoClient.Install(nodes.Get(m_clients[i]));
        clientApps.Start(Seconds(1.0));
        clientApps.Stop(Seconds(m_totalTime));
  
        std::cout << interfaces.GetAddress(m_clients[i]) << " ---> " << interfaces.GetAddress(m_servers[i]) << std::endl;
    

    }


}



int
main (int argc, char *argv[])
{
    AODVExample aodv;
    aodv.CommandSetUp(argc, argv);
    aodv.Run();
    return 0;
}
