// ====================== MODULES USED ======================
// These libraries provide all networking, mobility and simulation features.

#include "ns3/core-module.h"        // Simulation clock & events
#include "ns3/network-module.h"     // Nodes and packets
#include "ns3/mobility-module.h"    // Position of UAVs
#include "ns3/internet-module.h"    // IP addressing
#include "ns3/wifi-module.h"        // Wi-Fi communication
#include "ns3/applications-module.h"
#include "ns3/propagation-module.h" // Signal fading models
#include "ns3/flow-monitor-module.h"// Performance statistics

using namespace ns3;

std::string g_message = "Hello";   // Global message shared by all UAVs



// ====================== RECEIVE FUNCTION ======================
// This function acts like the "mailbox" of every UAV.
// Whenever a UAV receives a packet, this function prints its details.

void ReceivePacket(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;

    // Keep receiving packets until inbox becomes empty
    while ((packet = socket->RecvFrom(from)))
    {
        std::cout << Simulator::Now().GetSeconds()
                  << "s UAV RECEIVED packet of size "
                  << packet->GetSize() << " Bytes"
                  << std::endl;
    }
}



// ====================== SEND FUNCTION ======================
// This function creates and sends a packet to another UAV.

void SendPacket(Ptr<Socket> socket, Ipv4Address dst, uint16_t port, uint32_t id)
{
    // Convert message text into digital packet
    Ptr<Packet> packet = Create<Packet>((uint8_t*)g_message.c_str(), g_message.size());

    // Send packet to destination UAV
    socket->SendTo(packet, 0, InetSocketAddress(dst, port));

    // Print sending event
    std::cout << Simulator::Now().GetSeconds()
              << "s UAV-" << id << " SENT: "
              << g_message << std::endl;
}



// ====================== MAIN FUNCTION ======================
int main(int argc, char *argv[])
{
    // Default simulation parameters (can be changed via terminal)
    uint32_t nUav = 7;              // Number of drones
    double altitude = 100;          // Flying height (meters)
    std::string terrain = "less";   // Environment type
    std::string message = "Swarm Ready";

    // Allow parameters to be changed from command line
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



    // ====================== CREATE UAV NODES ======================
    NodeContainer nodes;
    nodes.Create(nUav);  // Create virtual drones



    // ====================== MOBILITY SETUP ======================
    // UAVs remain stationary at fixed altitude in a straight line

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    for (uint32_t i=0;i<nUav;i++)
    {
        nodes.Get(i)->GetObject<MobilityModel>()
        ->SetPosition(Vector(i*100,0,altitude)); // 100m spacing
    }



    // ====================== WIRELESS CHANNEL ======================
    // Creates the radio environment for communication

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();

    // Add fading if terrain has obstacles
    if (terrain == "more")
    {
        channel.AddPropagationLoss("ns3::NakagamiPropagationLossModel");
        channel.AddPropagationLoss("ns3::LogDistancePropagationLossModel");
        std::cout<<"Obstacle fading ENABLED\n";
    }



    // ====================== INSTALL WIFI DEVICES ======================
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ac); // High-speed Wi-Fi

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac"); // Direct UAV-to-UAV communication

    NetDeviceContainer devices = wifi.Install(phy,mac,nodes);



    // ====================== INSTALL INTERNET STACK ======================
    InternetStackHelper stack;
    stack.Install(nodes);



    // ====================== ASSIGN IP ADDRESSES ======================
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0","255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);



    // ====================== FLOW MONITOR ======================
    // Tracks network performance metrics

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();



    // ====================== CREATE RECEIVING SOCKETS ======================
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



    // ====================== SCHEDULE UAV MESSAGE SENDING ======================
    // UAVs send packets sequentially to the next UAV

    for(uint32_t i=0;i<nUav-1;i++)
    {
        Ptr<Socket> source = Socket::CreateSocket(nodes.Get(i),UdpSocketFactory::GetTypeId());

        Simulator::Schedule(Seconds(2+i), &SendPacket,
                            source, interfaces.GetAddress(i+1), port, i+1);
    }



    // ====================== RUN SIMULATION ======================
    Simulator::Stop(Seconds(15));
    Simulator::Run();



    // ====================== PRINT PERFORMANCE RESULTS ======================
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



    // ====================== CLEANUP ======================
    Simulator::Destroy();
    return 0;
}
