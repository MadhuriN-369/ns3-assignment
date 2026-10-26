// common.h - shared FlowMonitor reporting helper for all exercises
#ifndef COMMON_H
#define COMMON_H
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include <iostream>
#include <iomanip>
using namespace ns3;

// Prints one row per flow plus an aggregate row.
// Throughput = rxBytes*8 / (lastRx - firstTx)  [Mbps]
// Delay      = delaySum / rxPackets            [ms]
// PDR        = rx / tx * 100                   [%]
inline void
ReportFlows(Ptr<FlowMonitor> mon, FlowMonitorHelper& fh)
{
    mon->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> cls = DynamicCast<Ipv4FlowClassifier>(fh.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = mon->GetFlowStats();

    std::cout << "\n" << std::left << std::setw(5) << "Flow" << std::setw(30) << "Src:port -> Dst:port"
              << std::setw(6) << "Proto" << std::setw(8) << "Tx" << std::setw(8) << "Rx"
              << std::setw(8) << "Lost" << std::setw(9) << "PDR(%)" << std::setw(12) << "Thr(Mbps)"
              << std::setw(12) << "Delay(ms)" << "\n";

    double sumTx = 0, sumRx = 0, sumRxBytes = 0, sumDelay = 0;
    double minTx = 1e9, maxRx = 0;
    for (const auto& kv : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = cls->FindFlow(kv.first);
        const FlowMonitor::FlowStats& s = kv.second;
        double dur = (s.timeLastRxPacket - s.timeFirstTxPacket).GetSeconds();
        double thr = (dur > 0) ? s.rxBytes * 8.0 / dur / 1e6 : 0;
        double dly = s.rxPackets ? s.delaySum.GetSeconds() / s.rxPackets * 1000.0 : 0;
        double pdr = s.txPackets ? 100.0 * s.rxPackets / s.txPackets : 0;
        std::ostringstream od;
        od << t.sourceAddress << ":" << t.sourcePort << " -> " << t.destinationAddress << ":"
           << t.destinationPort;
        std::cout << std::left << std::setw(5) << kv.first << std::setw(30) << od.str()
                  << std::setw(6) << (t.protocol == 6 ? "TCP" : "UDP") << std::setw(8)
                  << s.txPackets << std::setw(8) << s.rxPackets << std::setw(8) << s.lostPackets
                  << std::fixed << std::setprecision(2) << std::setw(9) << pdr << std::setw(12)
                  << thr << std::setw(12) << dly << "\n";
        sumTx += s.txPackets; sumRx += s.rxPackets; sumRxBytes += s.rxBytes;
        sumDelay += s.delaySum.GetSeconds();
        minTx = std::min(minTx, s.timeFirstTxPacket.GetSeconds());
        maxRx = std::max(maxRx, s.timeLastRxPacket.GetSeconds());
    }
    double aggThr = (maxRx > minTx) ? sumRxBytes * 8.0 / (maxRx - minTx) / 1e6 : 0;
    std::cout << "AGGREGATE: tx=" << sumTx << " rx=" << sumRx << " PDR="
              << (sumTx ? 100.0 * sumRx / sumTx : 0) << "% throughput=" << aggThr
              << " Mbps avg delay=" << (sumRx ? sumDelay / sumRx * 1000 : 0) << " ms\n";
}
#endif
