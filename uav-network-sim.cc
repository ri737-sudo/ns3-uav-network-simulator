#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/propagation-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

std::string g_message = "Hello";


// ================= RECEIVE FUNCTION =================
void ReceivePacket(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        std::cout << Simulator::Now().GetSeconds()
                  << "s 🛰️ UAV RECEIVED packet of size "
                  << packet->GetSize() << " Bytes"
                  << std::endl;
    }
}


// ================= SEND FUNCTION =================
void SendPacket(Ptr<Socket> socket, Ipv4Address dst, uint16_t port, uint32_t id)
{
    Ptr<Packet> packet = Create<Packet>((uint8_t*)g_message.c_str(), g_message.size());
    socket->SendTo(packet, 0, InetSocketAddress(dst, port));

    std::cout << Simulator::Now().GetSeconds()
              << "s 🚁 UAV-" << id << " SENT: "
              << g_message << std::endl;
}


// ================= MAIN =================
int main(int argc, char *argv[])
{
    uint32_t nUav = 7;
    double altitude = 100;
    std::string terrain = "less";
    std::string message = "Swarm Ready";

    CommandLine cmd;
    cmd.AddValue("nUav","Number of UAVs", nUav);
    cmd.AddValue("altitude","Flying altitude", altitude);
    cmd.AddValue("terrain","less/more obstacles", terrain);
    cmd.AddValue("msg","Message to send", message);
    cmd.Parse(argc, argv);
    g_message = message;

    std::cout << "\n===== UAV NETWORK SIM =====\n";
    std::cout << "UAVs: " << nUav << "\n";
    std::cout << "Altitude: " << altitude << " m\n";
    std::cout << "Terrain: " << terrain << "\n";
    std::cout << "Message: " << g_message << "\n";

    // ================= CREATE NODES =================
    NodeContainer nodes;
    nodes.Create(nUav);

    // ================= MOBILITY =================
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    for (uint32_t i=0;i<nUav;i++)
    {
        nodes.Get(i)->GetObject<MobilityModel>()
        ->SetPosition(Vector(i*100,0,altitude));
    }

    // ================= CHANNEL =================
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();

    if (terrain == "more")
    {
        channel.AddPropagationLoss("ns3::NakagamiPropagationLossModel"); // fading
        channel.AddPropagationLoss("ns3::LogDistancePropagationLossModel");
        std::cout<<"Obstacle fading ENABLED\n";
    }

    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ac);

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(phy,mac,nodes);

    // ================= INTERNET =================
    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0","255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // ================= FLOW MONITOR =================
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // ================= SOCKETS =================
    uint16_t port = 9999;
    std::vector<Ptr<Socket>> sockets;

    for(uint32_t i=0;i<nUav;i++)
    {
        Ptr<Socket> recvSink = Socket::CreateSocket(nodes.Get(i),UdpSocketFactory::GetTypeId());
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(),port);
        recvSink->Bind(local);
        recvSink->SetRecvCallback(MakeCallback(&ReceivePacket));
        sockets.push_back(recvSink);
    }

    // ================= SEND TRAFFIC =================
    for(uint32_t i=0;i<nUav-1;i++)
    {
        Ptr<Socket> source = Socket::CreateSocket(nodes.Get(i),UdpSocketFactory::GetTypeId());
        Simulator::Schedule(Seconds(2+i), &SendPacket,
                            source, interfaces.GetAddress(i+1), port, i+1);
    }

    Simulator::Stop(Seconds(15));
    Simulator::Run();


    // ================= FLOW RESULTS =================
    std::cout << "\n========= FLOW MONITOR RESULTS =========\n";

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());

    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    for (auto const &flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);

        std::cout << "\nFlow " << flow.first
                  << " (" << t.sourceAddress << " -> "
                  << t.destinationAddress << ")\n";

        std::cout << "Tx Packets: " << flow.second.txPackets << "\n";
        std::cout << "Rx Packets: " << flow.second.rxPackets << "\n";
        std::cout << "Lost Packets: "
                  << flow.second.txPackets - flow.second.rxPackets << "\n";

        if(flow.second.rxPackets>0)
        {
            std::cout << "Avg Delay: "
                      << flow.second.delaySum.GetSeconds()/flow.second.rxPackets
                      << " s\n";

            std::cout << "Throughput: "
                      << flow.second.rxBytes*8.0/
                         flow.second.timeLastRxPacket.GetSeconds()/1024/1024
                      << " Mbps\n";
        }
    }

    Simulator::Destroy();
    return 0;
}#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/propagation-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

std::string g_message = "Hello";


// ================= RECEIVE FUNCTION =================
void ReceivePacket(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        std::cout << Simulator::Now().GetSeconds()
                  << "s 🛰️ UAV RECEIVED packet of size "
                  << packet->GetSize() << " Bytes"
                  << std::endl;
    }
}


// ================= SEND FUNCTION =================
void SendPacket(Ptr<Socket> socket, Ipv4Address dst, uint16_t port, uint32_t id)
{
    Ptr<Packet> packet = Create<Packet>((uint8_t*)g_message.c_str(), g_message.size());
    socket->SendTo(packet, 0, InetSocketAddress(dst, port));

    std::cout << Simulator::Now().GetSeconds()
              << "s 🚁 UAV-" << id << " SENT: "
              << g_message << std::endl;
}


// ================= MAIN =================
int main(int argc, char *argv[])
{
    uint32_t nUav = 7;
    double altitude = 100;
    std::string terrain = "less";
    std::string message = "Swarm Ready";

    CommandLine cmd;
    cmd.AddValue("nUav","Number of UAVs", nUav);
    cmd.AddValue("altitude","Flying altitude", altitude);
    cmd.AddValue("terrain","less/more obstacles", terrain);
    cmd.AddValue("msg","Message to send", message);
    cmd.Parse(argc, argv);
    g_message = message;

    std::cout << "\n===== UAV NETWORK SIM =====\n";
    std::cout << "UAVs: " << nUav << "\n";
    std::cout << "Altitude: " << altitude << " m\n";
    std::cout << "Terrain: " << terrain << "\n";
    std::cout << "Message: " << g_message << "\n";

    // ================= CREATE NODES =================
    NodeContainer nodes;
    nodes.Create(nUav);

    // ================= MOBILITY =================
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    for (uint32_t i=0;i<nUav;i++)
    {
        nodes.Get(i)->GetObject<MobilityModel>()
        ->SetPosition(Vector(i*100,0,altitude));
    }

    // ================= CHANNEL =================
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();

    if (terrain == "more")
    {
        channel.AddPropagationLoss("ns3::NakagamiPropagationLossModel"); // fading
        channel.AddPropagationLoss("ns3::LogDistancePropagationLossModel");
        std::cout<<"Obstacle fading ENABLED\n";
    }

    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ac);

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(phy,mac,nodes);

    // ================= INTERNET =================
    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0","255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // ================= FLOW MONITOR =================
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // ================= SOCKETS =================
    uint16_t port = 9999;
    std::vector<Ptr<Socket>> sockets;

    for(uint32_t i=0;i<nUav;i++)
    {
        Ptr<Socket> recvSink = Socket::CreateSocket(nodes.Get(i),UdpSocketFactory::GetTypeId());
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(),port);
        recvSink->Bind(local);
        recvSink->SetRecvCallback(MakeCallback(&ReceivePacket));
        sockets.push_back(recvSink);
    }

    // ================= SEND TRAFFIC =================
    for(uint32_t i=0;i<nUav-1;i++)
    {
        Ptr<Socket> source = Socket::CreateSocket(nodes.Get(i),UdpSocketFactory::GetTypeId());
        Simulator::Schedule(Seconds(2+i), &SendPacket,
                            source, interfaces.GetAddress(i+1), port, i+1);
    }

    Simulator::Stop(Seconds(15));
    Simulator::Run();


    // ================= FLOW RESULTS =================
    std::cout << "\n========= FLOW MONITOR RESULTS =========\n";

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());

    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    for (auto const &flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);

        std::cout << "\nFlow " << flow.first
                  << " (" << t.sourceAddress << " -> "
                  << t.destinationAddress << ")\n";

        std::cout << "Tx Packets: " << flow.second.txPackets << "\n";
        std::cout << "Rx Packets: " << flow.second.rxPackets << "\n";
        std::cout << "Lost Packets: "
                  << flow.second.txPackets - flow.second.rxPackets << "\n";

        if(flow.second.rxPackets>0)
        {
            std::cout << "Avg Delay: "
                      << flow.second.delaySum.GetSeconds()/flow.second.rxPackets
                      << " s\n";

            std::cout << "Throughput: "
                      << flow.second.rxBytes*8.0/
                         flow.second.timeLastRxPacket.GetSeconds()/1024/1024
                      << " Mbps\n";
        }
    }

    Simulator::Destroy();
    return 0;
}
