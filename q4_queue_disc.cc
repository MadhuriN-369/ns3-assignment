// Q4: Sender -> Router -> Receiver, compare queue discs on the router's egress.
// Run: ./ns3 run "q4_queue_disc --queue=droptail"
//      ./ns3 run "q4_queue_disc --queue=red"
//      ./ns3 run "q4_queue_disc --queue=codel"
#include "common.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
using namespace ns3;

int main(int argc, char* argv[])
{
    std::string queue = "droptail", bw = "2Mbps", bdelay = "5ms", offered = "5Mbps";
    uint32_t qsize = 50;  // packets
    double simTime = 10.0;
    CommandLine cmd(__FILE__);
    cmd.AddValue("queue", "droptail | red | codel", queue);
    cmd.AddValue("qsize", "Queue limit (packets)", qsize);
    cmd.AddValue("offered", "UDP offered load", offered);
    cmd.AddValue("simTime", "Seconds", simTime);
    cmd.Parse(argc, argv);

    // Keep the NIC's own queue tiny so the queue disc is the real buffer
    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", StringValue("1p"));

    NodeContainer n; n.Create(3);  // 0 sender, 1 router, 2 receiver
    PointToPointHelper acc, bn;
    acc.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    acc.SetChannelAttribute("Delay", StringValue("1ms"));
    bn.SetDeviceAttribute("DataRate", StringValue(bw));
    bn.SetChannelAttribute("Delay", StringValue(bdelay));
    NetDeviceContainer d1 = acc.Install(n.Get(0), n.Get(1));
    NetDeviceContainer d2 = bn.Install(n.Get(1), n.Get(2));

    InternetStackHelper stack; stack.Install(n);

    // Queue disc on router egress (device 0 of d2). DropTail == FIFO queue disc.
    TrafficControlHelper tch;
    std::string qs = std::to_string(qsize) + "p";
    if (queue == "droptail")
        tch.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", StringValue(qs));
    else if (queue == "red")
        tch.SetRootQueueDisc("ns3::RedQueueDisc", "MaxSize", StringValue(qs),
                             "MinTh", DoubleValue(qsize * 0.2), "MaxTh", DoubleValue(qsize * 0.6),
                             "LinkBandwidth", StringValue(bw), "LinkDelay", StringValue(bdelay),
                             "MeanPktSize", UintegerValue(1052));
    else
        tch.SetRootQueueDisc("ns3::CoDelQueueDisc", "MaxSize", StringValue(qs));
    QueueDiscContainer qd = tch.Install(d2.Get(0));

    Ipv4AddressHelper a;
    a.SetBase("10.1.1.0", "255.255.255.0"); a.Assign(d1);
    a.SetBase("10.1.2.0", "255.255.255.0"); Ipv4InterfaceContainer i2 = a.Assign(d2);
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
    sink.Install(n.Get(2)).Start(Seconds(0));
    OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(i2.GetAddress(1), 9));
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    onoff.SetConstantRate(DataRate(offered), 1024);
    ApplicationContainer app = onoff.Install(n.Get(0));
    app.Start(Seconds(1.0)); app.Stop(Seconds(simTime));

    FlowMonitorHelper fh; Ptr<FlowMonitor> mon = fh.InstallAll();
    Simulator::Stop(Seconds(simTime + 2));
    Simulator::Run();
    QueueDisc::Stats st = qd.Get(0)->GetStats();
    std::cout << "Q4: queue=" << queue << " limit=" << qsize << "p bottleneck=" << bw
              << " offered=" << offered << "\n"
              << "Queue-disc enqueued=" << st.nTotalEnqueuedPackets
              << " dropped=" << st.nTotalDroppedPackets << "\n";
    ReportFlows(mon, fh);
    Simulator::Destroy();
    return 0;
}
