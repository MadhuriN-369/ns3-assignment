// Q10: Wired p2p path (A -> B) and Wi-Fi path (STA -> AP) run side by side with identical UDP traffic.
// Flows: 10.1.1.x = wired, 10.1.2.x = Wi-Fi. Run: ./ns3 run q10_wired_vs_wifi
#include "common.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
using namespace ns3;

int main(int argc, char* argv[])
{
    std::string offered = "10Mbps", wiredRate = "20Mbps";
    double simTime = 10.0, distance = 10.0;
    CommandLine cmd(__FILE__);
    cmd.AddValue("offered", "UDP offered load (both scenarios)", offered);
    cmd.AddValue("wiredRate", "Wired link rate", wiredRate);
    cmd.AddValue("distance", "STA-AP distance (m)", distance);
    cmd.AddValue("simTime", "Seconds", simTime);
    cmd.Parse(argc, argv);

    // ---- Wired scenario ----
    NodeContainer w; w.Create(2);  // A, B
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue(wiredRate));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer wd = p2p.Install(w);

    // ---- Wi-Fi scenario ----
    NodeContainer sta, ap; sta.Create(1); ap.Create(1);
    YansWifiChannelHelper ch = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy; phy.SetChannel(ch.Create());
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    wifi.SetRemoteStationManager("ns3::IdealWifiManager");
    WifiMacHelper mac; Ssid ssid("ns3-lab");
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDev = wifi.Install(phy, mac, sta);
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDev = wifi.Install(phy, mac, ap);
    MobilityHelper mob;
    Ptr<ListPositionAllocator> pos = CreateObject<ListPositionAllocator>();
    pos->Add(Vector(0, 0, 0)); pos->Add(Vector(distance, 0, 0));
    mob.SetPositionAllocator(pos);
    mob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mob.Install(ap); mob.Install(sta);

    InternetStackHelper stack; stack.Install(w); stack.Install(ap); stack.Install(sta);
    Ipv4AddressHelper a;
    a.SetBase("10.1.1.0", "255.255.255.0"); Ipv4InterfaceContainer wIf = a.Assign(wd);
    a.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer apIf = a.Assign(apDev);
    a.Assign(staDev);

    // ---- Identical traffic ----
    auto install = [&](Ptr<Node> src, Ptr<Node> dst, Ipv4Address dstAddr) {
        PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
        sink.Install(dst).Start(Seconds(0));
        OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(dstAddr, 9));
        onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        onoff.SetConstantRate(DataRate(offered), 1024);
        ApplicationContainer app = onoff.Install(src);
        app.Start(Seconds(2.0)); app.Stop(Seconds(simTime));
    };
    install(w.Get(0), w.Get(1), wIf.GetAddress(1));       // wired
    install(sta.Get(0), ap.Get(0), apIf.GetAddress(0));   // Wi-Fi

    FlowMonitorHelper fh; Ptr<FlowMonitor> mon = fh.InstallAll();
    Simulator::Stop(Seconds(simTime + 2));
    Simulator::Run();
    std::cout << "Q10: offered=" << offered << " | wired=10.1.1.x (" << wiredRate
              << ") | Wi-Fi=10.1.2.x (802.11n, " << distance << " m)\n";
    ReportFlows(mon, fh);
    Simulator::Destroy();
    return 0;
}
