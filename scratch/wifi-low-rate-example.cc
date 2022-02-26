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

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/propagation-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv6-flow-classifier.h"
#include "ns3/flow-monitor-helper.h"
#include <ns3/lr-wpan-error-model.h>
#include "ns3/netanim-module.h"



using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WIFI-Low-Rate-Example");

class LowRateExample
{

public:
    LowRateExample();
    // run this experiment
    void Run();
    // configure script parameters
    void CommandSetUp(int argc, char * argv[]);
    // print arguments
    void PrintArguments();
    void PrintTime();
    
private:

// parameters

    // number of nodes in the network
    int m_nodes;
    // node coverage
    int m_coverage;
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
    Ipv6InterfaceContainer interfaces;

    // client nodes
    std::vector<int>m_clients;
    // server nodes
    std::vector<int>m_servers;

    // prefix for filenames
    std::string m_prefix;
    // int packets per second
    int m_packets_per_second;
    
    LrWpanHelper lrWpanHelper;
    FlowMonitorHelper flowmon;
    MobilityHelper mobility;
    InternetStackHelper internetv6;
    SixLowPanHelper sixLowPanHelper;
    NetDeviceContainer sixLowPanDevices;
    Ipv6AddressHelper ipv6;


// functions
    // create the nodes
    void CreateNodes();
    // create the devices
    void CreateDevices();
    // create network
    void CreateInternetStacks();
    // create application
    void InstallApplications();
    
    // receive packet 
    void ReceivePacket (Ptr<Socket> socket);
    // set up sink
    Ptr<Socket>SetupPacketReceive (Ipv6Address addr, Ptr<Node> node);



};


LowRateExample::LowRateExample():
    m_nodes(16),
    m_coverage(1),
    m_pause(0),
    m_totalTime(20),
    m_totalBytes(0),
    m_totalPackets(0),
    m_nflows(10),
    m_packetsSent(0),
    m_packetsReceived(0),
    m_packetsDropped(0),
    m_prefix("TaskA2"),
    m_packets_per_second(10)
    
{
   
}
void
LowRateExample::CommandSetUp(int argc, char *argv[])
{
    CommandLine cmd(__FILE__);
    cmd.AddValue("nodes", "Total number of nodes in the network", m_nodes);
    cmd.AddValue("coverage", "Coverage of a node", m_coverage);
    cmd.AddValue("pause", "Pause time for nodes movement", m_pause);
    cmd.AddValue("time", "Total time the simulation will run", m_totalTime);
    cmd.AddValue("flows", "Number of flows", m_nflows);
    cmd.AddValue("prefix", "Prefix of all generated file names\n", m_prefix);
    cmd.AddValue("packetsPerSecond", "Packets Sent per second", m_packets_per_second);
    cmd.Parse (argc, argv);
    return;
}

void
LowRateExample::PrintArguments()
{
    std::cout << "Simulation arguments:\n";
    std::cout << "nodes: " << m_nodes << "\n";
    std::cout << "flows: " << m_nflows << "\n";
    std::cout << "coverage: " << m_coverage << "\n";
    std::cout << "time: " << m_totalTime << "\n";
    std::cout << "pps: " << m_packets_per_second << "\n";
    std::cout << "--------------------------\n";
}
void
LowRateExample::PrintTime()
{
    std::cout << "Time: " << Simulator::Now().GetSeconds() << "\n";
    Simulator::Schedule(Seconds(1.0), &LowRateExample::PrintTime, this);

}
static inline std::string
PrintReceivedPacket (Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)
{
  std::ostringstream oss;

  oss << Simulator::Now ().GetSeconds () << " " << socket->GetNode ()->GetId ();

  if (InetSocketAddress::IsMatchingType (senderAddress))
    {
      InetSocketAddress addr = InetSocketAddress::ConvertFrom (senderAddress);
      oss << " received one packet from " << addr.GetIpv4 ();
    }
  else
    {
      oss << " received one packet!";
    }
  return oss.str ();
}

void
LowRateExample::ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address senderAddress;
  while ((packet = socket->RecvFrom (senderAddress)))
    {
      NS_LOG_UNCOND (PrintReceivedPacket (socket, packet, senderAddress));
    }
}
Ptr<Socket>
LowRateExample::SetupPacketReceive (Ipv6Address addr, Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  Inet6SocketAddress local = Inet6SocketAddress (addr, 9);
  sink->Bind (local);
  sink->SetRecvCallback (MakeCallback (&LowRateExample::ReceivePacket, this));

  return sink;
}


void 
LowRateExample::Run()
{

    
    PrintArguments();
    
    
    std::cout << "Setting networks...\n";
    CreateNodes();
    CreateDevices();
    CreateInternetStacks();
    InstallApplications();

    
    
    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

    NS_LOG_INFO("Running simulations...");
    
    Simulator::Stop(Seconds(m_totalTime + 5.0));

    Simulator::Schedule(Seconds(1.0), &LowRateExample::PrintTime, this);

    
    
    // setup animation
    // AnimationInterface anim (m_prefix + "-animation.xml");
    // anim.SetMaxPktsPerTraceFile(1000000);
    
    

    Simulator::Run();

    //this must be added after simulator::run()  
    
    int j=0;
    float AvgThroughput = 0;
    Time Jitter;
    Time Delay;
    // variables for output measurement
    uint32_t SentPackets = 0;
    uint32_t ReceivedPackets = 0;
    uint32_t DroppedPackets = 0;

    Ptr<Ipv6FlowClassifier> classifier = DynamicCast<Ipv6FlowClassifier> (flowmon.GetClassifier6 ());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
    
    std::ofstream flowOut("TaskA2/"+m_prefix+"-flowstats");
    bool printConsole = true;

    for (auto iter = stats.begin (); iter != stats.end (); ++iter) {
        Ipv6FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first); 
        

            SentPackets = SentPackets +(iter->second.txPackets);
            ReceivedPackets = ReceivedPackets + (iter->second.rxPackets);
            DroppedPackets += iter->second.txPackets - iter->second.rxPackets;
            AvgThroughput = AvgThroughput + iter->second.rxBytes;
            
            Delay = Delay + (iter->second.delaySum);
            
            
            j = j + 1;

            

    }

    AvgThroughput = AvgThroughput * 8.0 / ((m_totalTime-1.0) * 1024.0);
    
    if(ReceivedPackets)
    {
      Delay = Delay / ReceivedPackets;
    }
    
    flowOut << "--------Total Results of the simulation----------"<<std::endl << "\n";
    flowOut << "Nodes: " << m_nodes << "\n";
    flowOut << "Flows: " << m_nflows << "\n";
    flowOut << "pps: " << m_packets_per_second << "\n";
    flowOut << "coverage: " << m_coverage << "\n";
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
LowRateExample::CreateNodes()
{

    nodes.Create(m_nodes);
    
    // Name the nodes
    for(int j=0;j<m_nodes;j++){
        std::ostringstream os;
        os << "node-" << j;
        Names::Add(os.str(), nodes.Get(j));
    }



    // create mobility model
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (20),
                                 "DeltaY", DoubleValue (10),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (nodes);
    
    std::cout << "nodes created\n";

    
}
void 
LowRateExample::CreateDevices()
{


    // creating a channel with range propagation loss model  
    Config::SetDefault ("ns3::RangePropagationLossModel::MaxRange", DoubleValue (40 * m_coverage));
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel> ();
    Ptr<RangePropagationLossModel> propModel = CreateObject<RangePropagationLossModel> ();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
    channel->AddPropagationLossModel (propModel);
    channel->SetPropagationDelayModel (delayModel);

    
    // setting the channel in helper
    lrWpanHelper.SetChannel(channel);
    devices = lrWpanHelper.Install (nodes);

    lrWpanHelper.AssociateToPan (devices, 0);  
    std::cout << "devices created\n"; 



}
void 
LowRateExample::CreateInternetStacks()
{

    
    internetv6.Install (nodes);
    sixLowPanDevices = sixLowPanHelper.Install (devices);
    ipv6.SetBase (Ipv6Address ("2001:f00d::"), Ipv6Prefix (64));
    
    interfaces = ipv6.Assign (sixLowPanDevices);
    interfaces.SetForwarding (0, true);
    interfaces.SetDefaultRouteInAllNodes (0);

    for (uint32_t i = 0; i < sixLowPanDevices.GetN (); i++) {
      Ptr<NetDevice> dev = sixLowPanDevices.Get (i);
      dev->SetAttribute ("UseMeshUnder", BooleanValue (true));
      dev->SetAttribute ("MeshUnderRadius", UintegerValue (10));
    }

  std::cout << "stack installed\n"; 
    

}

void 
LowRateExample::InstallApplications()
{
    
    m_clients.reserve(m_nflows/2);
    m_servers.reserve(m_nflows/2);
    std::ofstream serverClient("TaskA2/" + m_prefix + "-server-client.txt");
    
    for(int i=0;i<m_nflows/2;i++)
    {
        m_clients[i] = i + m_nflows;
        m_servers[i] = i;
        serverClient << interfaces.GetAddress(m_servers[i], 1) << "\n";
        serverClient << interfaces.GetAddress(m_clients[i], 1) << "\n";
        
        
    }

    serverClient.close();

    
    OnOffHelper onoff1 ("ns3::TcpSocketFactory",Address ());
    std::ostringstream dr; 
    uint32_t packetSize = 1024;
    dr << packetSize << "B/s";
    onoff1.SetConstantRate (DataRate (dr.str()));
    onoff1.SetAttribute ("PacketSize", UintegerValue (packetSize));
    onoff1.SetAttribute ("MaxBytes", UintegerValue (20000 * packetSize));
  
    onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
    onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));

    
    std::cout << "Setting up flows " << std::endl;
    for (int i = 0; i < m_nflows/2; i++)
    {
      // these are receivers
      int s = m_servers[i];
      int c = m_clients[i];
      Ptr<Socket> sink = SetupPacketReceive (interfaces.GetAddress (s, 1), nodes.Get (s));

      AddressValue remoteAddress (Inet6SocketAddress (interfaces.GetAddress (s, 1), 9));
      onoff1.SetAttribute ("Remote", remoteAddress);

      Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
      // these are senders
      ApplicationContainer temp = onoff1.Install (nodes.Get (c));
      temp.Start (Seconds (1.0));
      temp.Stop (Seconds (m_totalTime));

      std::cout << interfaces.GetAddress(c, 1) << " ===> " << interfaces.GetAddress(s, 1) << "\n";
    }



  
}

int
main (int argc, char *argv[])
{
    LowRateExample aodv;
    aodv.CommandSetUp(argc, argv);
    aodv.Run();
    return 0;
}
