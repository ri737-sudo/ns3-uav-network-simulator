#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/propagation-module.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("UavNetworkSim");


// -------- GLOBALS (needed for scheduler) ----------
NodeContainer g_nodes;
Ipv4InterfaceContainer g_interfaces;
uint16_t g_port = 9000;


// -------- TOPOLOGY CHOOSER ----------
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
        uint32_t nodeId = socket->GetNode()->GetId();

        std::cout << Simulator::Now().GetSeconds()
                  << "s  🛰️  UAV-" << nodeId+1
                  << " RECEIVED a message"
                  << std::endl;
    }
}


// -------- SEND FUNCTION (scheduler safe) ----------
void SendUavMessage(uint32_t sender, uint32_t receiver)
{
    Ptr<Socket> source =
        Socket::CreateSocket(g_nodes.Get(sender), TcpSocketFactory::GetTypeId());

    InetSocketAddress remote =
        InetSocketAddress(g_interfaces.GetAddress(receiver), g_port);

    source->Connect(remote);

    std::string msg = "Hello from UAV " + std::to_string(sender+1);
    Ptr<Packet> packet = Create<Packet>((uint8_t*)msg.c_str(), msg.length());

    source->Send(packet);

    std::cout << Simulator::Now().GetSeconds()
              << "s  🚁 UAV-" << sender+1
              << " SENT message to UAV-" << receiver+1 << std::endl;
}


// ================= MAIN =================
int main(int argc, char *argv[])
{
    uint32_t nUav = 5;
    double altitude = 100.0;
    std::string emergency = "no";
    std::string terrain = "less";

    CommandLine cmd;
    cmd.AddValue("nUav", "Number of drones", nUav);
    cmd.AddValue("altitude", "Flying altitude", altitude);
    cmd.AddValue("emergency", "yes/no", emergency);
    cmd.AddValue("terrain", "less/more obstacles", terrain);
    cmd.Parse(argc, argv);

    std::string protocol = (emergency == "yes") ? "TCP" : "UDP";

    std::cout << "\n===== UAV NETWORK NS3 SIM =====\n";
    std::cout << "UAV Count: " << nUav << std::endl;
    std::cout << "Altitude: " << altitude << " m\n";
    std::cout << "Topology: " << ChooseTopology(nUav) << std::endl;
    std::cout << "Protocol: " << protocol << std::endl;

    // -------- CREATE NODES ----------
    NodeContainer nodes;
    nodes.Create(nUav);

    // -------- WIFI CHANNEL ----------
    YansWifiChannelHelper channel;
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");

    if (terrain == "more")
        channel.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                                   "Exponent", DoubleValue(4.0));
    else
        channel.AddPropagationLoss("ns3::FriisPropagationLossModel");

    Ptr<YansWifiChannel> wifiChannel = channel.Create();

    YansWifiPhyHelper phy;
    phy.SetChannel(wifiChannel);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ac);

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    // -------- MOBILITY ----------
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    for (uint32_t i = 0; i < nUav; i++)
    {
        Ptr<MobilityModel> mob = nodes.Get(i)->GetObject<MobilityModel>();
        mob->SetPosition(Vector(i * 50, 0, altitude));
    }

    // -------- INTERNET ----------
    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper ip;
    ip.SetBase("10.1.0.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ip.Assign(devices);

    uint16_t port = 9000;

    // Save globals for scheduler
    g_nodes = nodes;
    g_interfaces = interfaces;
    g_port = port;

    // -------- CREATE RECEIVERS ON ALL UAVs ----------
    for(uint32_t i=0; i<nUav; i++)
    {
        Ptr<Socket> recvSink =
            Socket::CreateSocket(nodes.Get(i), TcpSocketFactory::GetTypeId());

        InetSocketAddress local =
            InetSocketAddress(Ipv4Address::GetAny(), port);

        recvSink->Bind(local);
        recvSink->Listen();
        recvSink->SetRecvCallback(MakeCallback(&ReceivePacket));
    }

    // -------- SCHEDULE UAV COMMUNICATION ----------
    double startTime = 2.0;
    double timeGap = 1.0;

    for(uint32_t i=0; i<nUav-1; i++)
    {
        Simulator::Schedule(Seconds(startTime + i*timeGap),
                            &SendUavMessage, i, i+1);
    }

    // -------- SIMULATION TIME ----------
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();
}
