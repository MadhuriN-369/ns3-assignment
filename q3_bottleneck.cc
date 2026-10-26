// Q3: Two senders -> router -> receiver; last link is the bottleneck.
// Run: ./ns3 run "q3_bottleneck --bottleneck=10Mbps"
//      ./ns3 run "q3_bottleneck --bottleneck=1Mbps"
#include "common.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
using namespace ns3;

int main(int argc, char* argv[])
{
    std::string bottleneck = "10Mbps", perSender = "4Mbps";
    double simTime = 10.0;
    CommandLine cmd(__FILE__);
    cmd.AddValue("bottleneck", "Router->receiver link rate", bottleneck);
    cmd.AddValue("perSender", "Offered rate per sender", perSender);
    cmd.AddValue("simTime", "Seconds", simTime);
    cmd.Parse(argc, argv);

    NodeContainer n; n.Create(4);  // 0 SenderA, 1 SenderB, 2 Router, 3 Receiver
    PointToPointHelper access, bn;
    access.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    access.SetChannelAttribute("Delay", StringValue("2ms"));
    bn.SetDeviceAttribute("DataRate", StringValue(bottleneck));
    bn.SetChannelAttribute("Delay", StringValue("5ms"));
    NetDeviceContainer dA = access.Install(n.Get(0), n.Get(2));
    NetDeviceContainer dB = access.Install(n.Get(1), n.Get(2));
    NetDeviceContainer dR = bn.Install(n.Get(2), n.Get(3));

    InternetStackHelper stack; stack.Install(n);
    Ipv4AddressHelper a;
    a.SetBase("10.1.1.0", "255.255.255.0"); a.Assign(dA);
    a.SetBase("10.1.2.0", "255.255.255.0"); a.Assign(dB);
    a.SetBase("10.1.3.0", "255.255.255.0"); Ipv4InterfaceContainer iR = a.Assign(dR);
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t port = 9;
    PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer s = sink.Install(n.Get(3));
    s.Start(Seconds(0));

    OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(iR.GetAddress(1), port));
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    onoff.SetConstantRate(DataRate(perSender), 1024);
    ApplicationContainer apps;
    apps.Add(onoff.Install(n.Get(0)));
    apps.Add(onoff.Install(n.Get(1)));
    apps.Start(Seconds(1.0)); apps.Stop(Seconds(simTime));

    FlowMonitorHelper fh; Ptr<FlowMonitor> mon = fh.InstallAll();
    Simulator::Stop(Seconds(simTime + 2));
    Simulator::Run();
    std::cout << "Q3: bottleneck=" << bottleneck << " offered=2 x " << perSender << "\n";
    ReportFlows(mon, fh);
    Simulator::Destroy();
    return 0;
}
