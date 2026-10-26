// Q6: CBR UDP over p2p; compare offered rate below vs above link capacity.
// Run: ./ns3 run "q6_udp_rate_pdr --offered=3Mbps"
//      ./ns3 run "q6_udp_rate_pdr --offered=8Mbps"
#include "common.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
using namespace ns3;

int main(int argc, char* argv[])
{
    std::string rate = "5Mbps", offered = "3Mbps";
    double simTime = 10.0;
    CommandLine cmd(__FILE__);
    cmd.AddValue("rate", "Link capacity", rate);
    cmd.AddValue("offered", "UDP offered rate", offered);
    cmd.AddValue("simTime", "Seconds", simTime);
    cmd.Parse(argc, argv);

    NodeContainer n; n.Create(2);
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue(rate));
    p2p.SetChannelAttribute("Delay", StringValue("5ms"));
    NetDeviceContainer d = p2p.Install(n);
    InternetStackHelper stack; stack.Install(n);
    Ipv4AddressHelper a; a.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifc = a.Assign(d);

    PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
    ApplicationContainer s = sink.Install(n.Get(1));
    s.Start(Seconds(0));
    OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(ifc.GetAddress(1), 9));
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    onoff.SetConstantRate(DataRate(offered), 1024);
    ApplicationContainer app = onoff.Install(n.Get(0));
    app.Start(Seconds(1.0)); app.Stop(Seconds(simTime));

    FlowMonitorHelper fh; Ptr<FlowMonitor> mon = fh.InstallAll();
    Simulator::Stop(Seconds(simTime + 2));
    Simulator::Run();
    std::cout << "Q6: capacity=" << rate << " offered=" << offered
              << "  (PDR = received / sent x 100%)\n";
    ReportFlows(mon, fh);
    Simulator::Destroy();
    return 0;
}
