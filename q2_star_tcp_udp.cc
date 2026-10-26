// Q2: Star topology. Hub(0) connected to Sender1(1, TCP), Sender2(2, UDP), Receiver(3).
// Run: ./ns3 run q2_star_tcp_udp
#include "common.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
using namespace ns3;

int main(int argc, char* argv[])
{
    double simTime = 10.0;
    std::string udpRate = "5Mbps";
    CommandLine cmd(__FILE__);
    cmd.AddValue("udpRate", "UDP offered rate", udpRate);
    cmd.AddValue("simTime", "Seconds", simTime);
    cmd.Parse(argc, argv);

    NodeContainer n; n.Create(4);  // 0 hub, 1 sender1 (TCP), 2 sender2 (UDP), 3 receiver
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer d1 = p2p.Install(n.Get(0), n.Get(1));
    NetDeviceContainer d2 = p2p.Install(n.Get(0), n.Get(2));
    NetDeviceContainer d3 = p2p.Install(n.Get(0), n.Get(3));

    InternetStackHelper stack; stack.Install(n);
    Ipv4AddressHelper a;
    a.SetBase("10.1.1.0", "255.255.255.0"); a.Assign(d1);
    a.SetBase("10.1.2.0", "255.255.255.0"); a.Assign(d2);
    a.SetBase("10.1.3.0", "255.255.255.0"); Ipv4InterfaceContainer i3 = a.Assign(d3);
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    Ipv4Address rx = i3.GetAddress(1);

    // Receiver applications: one TCP sink, one UDP sink
    PacketSinkHelper tcpSink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 50000));
    PacketSinkHelper udpSink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 50001));
    ApplicationContainer tcpS = tcpSink.Install(n.Get(3));
    ApplicationContainer udpS = udpSink.Install(n.Get(3));
    tcpS.Start(Seconds(0)); udpS.Start(Seconds(0));

    // TCP flow: Sender1 -> Receiver (greedy)
    BulkSendHelper bulk("ns3::TcpSocketFactory", InetSocketAddress(rx, 50000));
    bulk.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer tcpApp = bulk.Install(n.Get(1));
    tcpApp.Start(Seconds(1.0)); tcpApp.Stop(Seconds(simTime));

    // UDP flow: Sender2 -> Receiver (constant rate)
    OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(rx, 50001));
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    onoff.SetConstantRate(DataRate(udpRate), 1024);
    ApplicationContainer udpApp = onoff.Install(n.Get(2));
    udpApp.Start(Seconds(1.0)); udpApp.Stop(Seconds(simTime));

    FlowMonitorHelper fh; Ptr<FlowMonitor> mon = fh.InstallAll();
    Simulator::Stop(Seconds(simTime + 2));
    Simulator::Run();

    double dur = simTime - 1.0;
    uint64_t tb = DynamicCast<PacketSink>(tcpS.Get(0))->GetTotalRx();
    uint64_t ub = DynamicCast<PacketSink>(udpS.Get(0))->GetTotalRx();
    std::cout << "Q2: TCP bytes received = " << tb << " (" << tb * 8.0 / dur / 1e6 << " Mbps)\n"
              << "    UDP bytes received = " << ub << " (" << ub * 8.0 / dur / 1e6 << " Mbps)\n";
    ReportFlows(mon, fh);  // reverse TCP flow = ACKs
    Simulator::Destroy();
    return 0;
}
