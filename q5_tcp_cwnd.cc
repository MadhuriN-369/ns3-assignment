// Q5: TCP sender -> receiver over p2p; trace cwnd to q5-cwnd.dat (time[s]  cwnd[bytes]).
// Run: ./ns3 run "q5_tcp_cwnd" then: python3 plot_cwnd.py q5-cwnd.dat
#include "common.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
using namespace ns3;

static void CwndChange(Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << newCwnd << std::endl;
}

static void ConnectCwnd(Ptr<OutputStreamWrapper> stream)
{
    Config::ConnectWithoutContext("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
                                  MakeBoundCallback(&CwndChange, stream));
}

int main(int argc, char* argv[])
{
    std::string tcp = "TcpNewReno", rate = "5Mbps", delay = "10ms";
    uint32_t qsize = 20;
    double simTime = 30.0;
    CommandLine cmd(__FILE__);
    cmd.AddValue("tcp", "TcpNewReno | TcpCubic | TcpLinuxReno ...", tcp);
    cmd.AddValue("rate", "Link rate", rate);
    cmd.AddValue("qsize", "Sender queue limit (packets) - small to force loss", qsize);
    cmd.AddValue("simTime", "Seconds", simTime);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::" + tcp));
    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", StringValue("1p"));

    NodeContainer n; n.Create(2);
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue(rate));
    p2p.SetChannelAttribute("Delay", StringValue(delay));
    NetDeviceContainer d = p2p.Install(n);

    InternetStackHelper stack; stack.Install(n);
    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", StringValue(std::to_string(qsize) + "p"));
    tch.Install(d);
    Ipv4AddressHelper a; a.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifc = a.Assign(d);

    PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 50000));
    ApplicationContainer s = sink.Install(n.Get(1));
    s.Start(Seconds(0));
    BulkSendHelper bulk("ns3::TcpSocketFactory", InetSocketAddress(ifc.GetAddress(1), 50000));
    bulk.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer app = bulk.Install(n.Get(0));
    app.Start(Seconds(1.0)); app.Stop(Seconds(simTime));

    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream("q5-cwnd.dat");
    Simulator::Schedule(Seconds(1.00001), &ConnectCwnd, stream);  // socket exists after app start

    Simulator::Stop(Seconds(simTime + 1));
    Simulator::Run();
    uint64_t rx = DynamicCast<PacketSink>(s.Get(0))->GetTotalRx();
    std::cout << "Q5: " << tcp << " received " << rx << " bytes ("
              << rx * 8.0 / (simTime - 1) / 1e6 << " Mbps). Trace: q5-cwnd.dat\n";
    Simulator::Destroy();
    return 0;
}
