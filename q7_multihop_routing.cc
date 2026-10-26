// Q7: Source - R1 - R2 - R3 - Dest, one /24 per link, static routes, fault injection.
// Run: ./ns3 run "q7_multihop_routing --fault=0"   (healthy)
//      ./ns3 run "q7_multihop_routing --fault=1"   (R2 missing route to Dest subnet, fixed at --fixAt)
//      ./ns3 run "q7_multihop_routing --fault=2"   (R2 route with wrong next hop/interface, fixed at --fixAt)
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <iostream>
using namespace ns3;

int main(int argc, char* argv[])
{
    uint32_t fault = 0;
    double fixAt = 5.0, simTime = 10.0;
    CommandLine cmd(__FILE__);
    cmd.AddValue("fault", "0=none 1=missing route 2=wrong next hop", fault);
    cmd.AddValue("fixAt", "Time (s) the route is repaired", fixAt);
    cmd.Parse(argc, argv);

    const char* names[5] = {"Source", "R1", "R2", "R3", "Dest"};
    NodeContainer n; n.Create(5);
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer d[4];
    for (int i = 0; i < 4; i++) d[i] = p2p.Install(n.Get(i), n.Get(i + 1));
    InternetStackHelper stack; stack.Install(n);

    Ipv4AddressHelper a;
    Ipv4InterfaceContainer ifc[4];
    for (int i = 0; i < 4; i++)
    {
        std::ostringstream s; s << "10.0." << (i + 1) << ".0";
        a.SetBase(Ipv4Address(s.str().c_str()), Ipv4Mask("255.255.255.0"));
        ifc[i] = a.Assign(d[i]);
    }

    // ---- Addressing table ----
    std::cout << "Node    If  IP address\n";
    for (int i = 0; i < 5; i++)
    {
        Ptr<Ipv4> ip = n.Get(i)->GetObject<Ipv4>();
        for (uint32_t j = 1; j < ip->GetNInterfaces(); j++)
            std::cout << names[i] << "\t" << j << "   " << ip->GetAddress(j, 0).GetLocal()
                      << "/24\n";
    }

    // ---- Static routes (interface 0 is loopback) ----
    Ipv4StaticRoutingHelper srh;
    Ipv4Mask m("255.255.255.0");
    auto SR = [&](int i) { return srh.GetStaticRouting(n.Get(i)->GetObject<Ipv4>()); };
    SR(0)->SetDefaultRoute(Ipv4Address("10.0.1.2"), 1);
    SR(4)->SetDefaultRoute(Ipv4Address("10.0.4.1"), 1);
    // R1: forward direction
    SR(1)->AddNetworkRouteTo(Ipv4Address("10.0.3.0"), m, Ipv4Address("10.0.2.2"), 2);
    SR(1)->AddNetworkRouteTo(Ipv4Address("10.0.4.0"), m, Ipv4Address("10.0.2.2"), 2);
    // R2: return direction + forward direction (the one we break)
    SR(2)->AddNetworkRouteTo(Ipv4Address("10.0.1.0"), m, Ipv4Address("10.0.2.1"), 1);
    Ptr<Ipv4StaticRouting> r2 = SR(2);
    uint32_t badIdx = 0;
    if (fault == 0)
        r2->AddNetworkRouteTo(Ipv4Address("10.0.4.0"), m, Ipv4Address("10.0.3.2"), 2);
    else if (fault == 2)
    {
        r2->AddNetworkRouteTo(Ipv4Address("10.0.4.0"), m, Ipv4Address("10.0.2.1"), 1);  // wrong
        badIdx = r2->GetNRoutes() - 1;
    }
    if (fault != 0)
        Simulator::Schedule(Seconds(fixAt), [=]() {
            if (fault == 2) r2->RemoveRoute(badIdx);
            r2->AddNetworkRouteTo(Ipv4Address("10.0.4.0"), Ipv4Mask("255.255.255.0"),
                                  Ipv4Address("10.0.3.2"), 2);
            std::cout << "[t=" << Simulator::Now().GetSeconds() << "s] R2 route FIXED\n";
        });
    // R3: return direction
    SR(3)->AddNetworkRouteTo(Ipv4Address("10.0.1.0"), m, Ipv4Address("10.0.3.1"), 1);
    SR(3)->AddNetworkRouteTo(Ipv4Address("10.0.2.0"), m, Ipv4Address("10.0.3.1"), 1);

    // ---- Traffic ----
    PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
    ApplicationContainer s = sink.Install(n.Get(4));
    s.Start(Seconds(0));
    OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(ifc[3].GetAddress(1), 9));
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    onoff.SetConstantRate(DataRate("1Mbps"), 512);
    ApplicationContainer app = onoff.Install(n.Get(0));
    app.Start(Seconds(1.0)); app.Stop(Seconds(simTime - 1));

    Ptr<PacketSink> ps = DynamicCast<PacketSink>(s.Get(0));
    double probe = (fault != 0) ? fixAt - 0.1 : simTime / 2;
    Simulator::Schedule(Seconds(probe), [ps, probe]() {
        std::cout << "[t=" << probe << "s] Dest received " << ps->GetTotalRx() << " bytes\n";
    });
    p2p.EnablePcapAll("q7");  // pcap evidence (q7-<node>-<if>.pcap)
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    std::cout << "[t=end] Dest received " << ps->GetTotalRx() << " bytes\n";
    Simulator::Destroy();
    return 0;
}
