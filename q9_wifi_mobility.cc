// Q9: 1 AP + 1 mobile STA. speed=0 -> stays near AP; speed>0 -> moves away along +x.
// Run: ./ns3 run "q9_wifi_mobility --speed=0"
//      ./ns3 run "q9_wifi_mobility --speed=10"
#include "common.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include <cmath>
using namespace ns3;

static uint64_t g_last = 0;
static void Sample(Ptr<PacketSink> sink, Ptr<MobilityModel> mm)
{
    uint64_t now = sink->GetTotalRx();
    Vector p = mm->GetPosition();
    std::cout << std::fixed << std::setprecision(1) << "t=" << Simulator::Now().GetSeconds()
              << "s  dist=" << std::sqrt(p.x * p.x + p.y * p.y) << " m  thr="
              << (now - g_last) * 8.0 / 1e6 << " Mbps\n";
    g_last = now;
    Simulator::Schedule(Seconds(1.0), &Sample, sink, mm);
}

int main(int argc, char* argv[])
{
    double speed = 0.0, startDist = 5.0, simTime = 20.0;
    std::string offered = "10Mbps";
    CommandLine cmd(__FILE__);
    cmd.AddValue("speed", "STA speed along +x (m/s); 0 = stationary", speed);
    cmd.AddValue("startDist", "Initial distance from AP (m)", startDist);
    cmd.AddValue("offered", "UDP offered load", offered);
    cmd.AddValue("simTime", "Seconds", simTime);
    cmd.Parse(argc, argv);

    NodeContainer sta, ap; sta.Create(1); ap.Create(1);
    YansWifiChannelHelper ch = YansWifiChannelHelper::Default();  // log-distance loss
    YansWifiPhyHelper phy; phy.SetChannel(ch.Create());
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    wifi.SetRemoteStationManager("ns3::IdealWifiManager");
    WifiMacHelper mac; Ssid ssid("ns3-lab");
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDev = wifi.Install(phy, mac, sta);
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDev = wifi.Install(phy, mac, ap);

    MobilityHelper apMob;
    apMob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    apMob.Install(ap);  // AP at origin
    MobilityHelper staMob;
    staMob.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    staMob.Install(sta);
    Ptr<ConstantVelocityMobilityModel> mm = sta.Get(0)->GetObject<ConstantVelocityMobilityModel>();
    mm->SetPosition(Vector(startDist, 0, 0));
    mm->SetVelocity(Vector(speed, 0, 0));

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
    ApplicationContainer app = onoff.Install(sta.Get(0));
    app.Start(Seconds(2.0)); app.Stop(Seconds(simTime));

    Simulator::Schedule(Seconds(3.0), &Sample, DynamicCast<PacketSink>(s.Get(0)), mm);
    FlowMonitorHelper fh; Ptr<FlowMonitor> mon = fh.InstallAll();
    Simulator::Stop(Seconds(simTime + 2));
    Simulator::Run();
    std::cout << "Q9: speed=" << speed << " m/s startDist=" << startDist << " m offered=" << offered << "\n";
    ReportFlows(mon, fh);
    Simulator::Destroy();
    return 0;
}
