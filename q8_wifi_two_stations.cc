// Q8: 1 AP + 2 STAs (802.11n), both STAs send UDP to a sink on the AP.
// Run: ./ns3 run "q8_wifi_two_stations --offered=5Mbps"    (light load)
//      ./ns3 run "q8_wifi_two_stations --offered=40Mbps"   (saturated)
//      ./ns3 run "q8_wifi_two_stations --distance=40"      (position change)
#include "common.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
using namespace ns3;

int main(int argc, char* argv[])
{
    std::string offered = "5Mbps";
    double distance = 5.0, simTime = 10.0;
    CommandLine cmd(__FILE__);
    cmd.AddValue("offered", "Offered load PER STATION", offered);
    cmd.AddValue("distance", "STA distance from AP (m)", distance);
    cmd.AddValue("simTime", "Seconds", simTime);
    cmd.Parse(argc, argv);

    NodeContainer sta, ap; sta.Create(2); ap.Create(1);
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
    pos->Add(Vector(0, 0, 0));         // AP
    pos->Add(Vector(distance, 0, 0));  // STA1
    pos->Add(Vector(0, distance, 0));  // STA2
    mob.SetPositionAllocator(pos);
    mob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mob.Install(ap); mob.Install(sta);

    InternetStackHelper stack; stack.Install(ap); stack.Install(sta);
    Ipv4AddressHelper a; a.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer apIf = a.Assign(apDev);
    a.Assign(staDev);

    PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
    ApplicationContainer s = sink.Install(ap.Get(0));
    s.Start(Seconds(0));
    OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(apIf.GetAddress(0), 9));
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    onoff.SetConstantRate(DataRate(offered), 1024);
    ApplicationContainer apps = onoff.Install(sta);
    apps.Start(Seconds(2.0)); apps.Stop(Seconds(simTime));  // wait for association

    FlowMonitorHelper fh; Ptr<FlowMonitor> mon = fh.InstallAll();
    Simulator::Stop(Seconds(simTime + 2));
    Simulator::Run();
    std::cout << "Q8: 802.11n, 2 STAs, offered=" << offered << " each, distance=" << distance << " m\n";
    ReportFlows(mon, fh);
    Simulator::Destroy();
    return 0;
}
