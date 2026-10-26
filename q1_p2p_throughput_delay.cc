// Q1: Two nodes, point-to-point link, UDP A -> B.
// Run:  ./ns3 run "q1_p2p_throughput_delay --rate=5Mbps"
//       ./ns3 run "q1_p2p_throughput_delay --rate=10Mbps"
#include "common.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
using namespace ns3;

int main(int argc, char* argv[])
{
    std::string rate = "5Mbps", delay = "2ms", offered = "8Mbps";
    double simTime = 10.0;
    uint32_t pkt = 1024;
    CommandLine cmd(__FILE__);
    cmd.AddValue("rate", "Link data rate", rate);
    cmd.AddValue("delay", "Propagation delay (keep constant)", delay);
    cmd.AddValue("offered", "UDP offered load", offered);
    cmd.AddValue("simTime", "Seconds", simTime);
    cmd.Parse(argc, argv);

    NodeContainer nodes; nodes.Create(2);
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue(rate));
    p2p.SetChannelAttribute("Delay", StringValue(delay));
    NetDeviceContainer dev = p2p.Install(nodes);

    InternetStackHelper stack; stack.Install(nodes);
    Ipv4AddressHelper addr; addr.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifc = addr.Assign(dev);

    uint16_t port = 9;
    PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sink.Install(nodes.Get(1));
    sinkApp.Start(Seconds(0.0)); sinkApp.Stop(Seconds(simTime + 1));

    OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(ifc.GetAddress(1), port));
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    onoff.SetConstantRate(DataRate(offered), pkt);
    ApplicationContainer app = onoff.Install(nodes.Get(0));
    app.Start(Seconds(1.0)); app.Stop(Seconds(simTime));

    FlowMonitorHelper fh; Ptr<FlowMonitor> mon = fh.InstallAll();
    Simulator::Stop(Seconds(simTime + 2));
    Simulator::Run();
    std::cout << "Q1: link=" << rate << " delay=" << delay << " offered=" << offered << "\n";
    ReportFlows(mon, fh);
    Simulator::Destroy();
    return 0;
}
