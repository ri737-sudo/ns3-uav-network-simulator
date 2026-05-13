#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/propagation-module.h"
#include "ns3/socket.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("UavNetworkSim");

// -------- GLOBALS ----------
NodeContainer g_nodes;
Ipv4InterfaceContainer g_interfaces;
uint16_t g_port = 9000;

// -------- TOPOLOGY ----------
std::string ChooseTopology(uint32_t n)
{
    if (n == 1) return "Single UAV";
    else if (n <= 3) return "Point-to-Point";
    else if (n <= 6) return "Star";
    else if (n <= 12) return "Mesh";
    else return "Hierarchical Cluster";
}

// -------- RECEIVE CALLBACK ----------
void ReceivePacket(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        std::cout << Simulator::Now().GetSeconds()
                  << "s  🛰️ UAV RECEIVED packet of size "
                  << packet->GetSize()
                  << std::endl;
    }
}

// -------- SEND FUNCTION ----------
void SendUavMessage(uint32_t sender, uint32_t receiver)
{
    Ptr<Socket> source =
        Socket::CreateSocket(g_nodes.Get(sender), UdpSocketFactory::GetTypeId());

    InetSocketAddress remote =
        InetSocketAddress(g_interfaces.GetAddress(receiver), g_port);

    source->Connect(remote);

    std::string msg = "Hello from UAV " + std::to_string(sender);

    Ptr<Packet> packet = Create<Packet>((uint8_t*)msg.c_str(), msg.size());

    source->Send(packet);

    std::cout << Simulator::Now().GetSeconds()
              << "s 🚁 UAV-" << sender
              << " sent to UAV-" << receiver << std::endl;
}

// ================= MAIN =================
int main(int argc, char *argv[])
{
    // ---- DEFAULT VALUES ----
    uint32_t nUav = 5;
    double altitude = 100.0;
    std::string emergency = "no";
    std::string terrain = "less";

    // ---- COMMAND LINE ----
    CommandLine cmd;
    cmd.AddValue("nUav", "Number of drones", nUav);
    cmd.AddValue("altitude", "Flying altitude", altitude);
    cmd.AddValue("emergency", "yes/no", emergency);
    cmd.AddValue("terrain", "less/more obstacles", terrain);
    cmd.Parse(argc, argv);

    std::cout << "\n===== UAV NETWORK SIM =====\n";
    std::cout << "UAVs: " << nUav << "\n";
    std::cout << "Altitude: " << altitude << "\n";
    std::cout << "Topology: " << ChooseTopology(nUav) << "\n";
    std::cout << "Emergency: " << emergency << "\n";
    std::cout << "Terrain: " << terrain << "\n";

    // ---- CREATE NODES ----
    g_nodes.Create(nUav);

    // ---- WIFI ----
    YansWifiChannelHelper channel;
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");

    if (terrain == "more")
        channel.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                                   "Exponent", DoubleValue(4.0));
    else
        channel.AddPropagationLoss("ns3::FriisPropagationLossModel");

    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ac);

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(phy, mac, g_nodes);

    // ---- MOBILITY ----
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(g_nodes);

    for (uint32_t i = 0; i < nUav; i++)
    {
        Ptr<MobilityModel> m = g_nodes.Get(i)->GetObject<MobilityModel>();
        m->SetPosition(Vector(i * 50, 0, altitude));
    }

    // ---- INTERNET ----
    InternetStackHelper internet;
    internet.Install(g_nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.0.0", "255.255.255.0");

    g_interfaces = ipv4.Assign(devices);

    g_port = 9000;

    // ---- RECEIVERS ----
    for (uint32_t i = 0; i < nUav; i++)
    {
        Ptr<Socket> sink =
            Socket::CreateSocket(g_nodes.Get(i), UdpSocketFactory::GetTypeId());

        InetSocketAddress local(Ipv4Address::GetAny(), g_port);

        sink->Bind(local);

        sink->SetRecvCallback(MakeCallback(&ReceivePacket));
    }

    // ---- SCHEDULING ----
    double startTime = 2.0;
    double interval = 1.0;

    for (uint32_t i = 0; i < nUav - 1; i++)
    {
        Simulator::Schedule(Seconds(startTime + i * interval),
                            &SendUavMessage, i, i + 1);
    }

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
