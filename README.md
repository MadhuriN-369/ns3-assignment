# NS-3 Lab – scripts for Q1–Q10

Tested target: ns-3.36+ (C++17, `WIFI_STANDARD_80211n`, `QueueDisc`). On older versions,
change the Wi-Fi standard enum and `DropTailQueue<Packet>` names as needed.

## Build / run
```
cp *.cc common.h plot_cwnd.py ~/ns-3-dev/scratch/
cd ~/ns-3-dev
./ns3 run "q1_p2p_throughput_delay --rate=5Mbps"
```
Every `--option` shown in a script's header comment is a command-line argument,
so the same binary produces both runs. Keep `simTime` identical when comparing.
Redirect output for your report: `./ns3 run "..." > q1_5mbps.txt`

## Run matrix
| Q | Runs |
|---|------|
| 1 | `--rate=5Mbps`, `--rate=10Mbps` (delay fixed, offered 8 Mbps) |
| 2 | default |
| 3 | `--bottleneck=10Mbps`, `--bottleneck=1Mbps` |
| 4 | `--queue=droptail`, `--queue=red` (also `codel`) |
| 5 | default, then `python3 plot_cwnd.py q5-cwnd.dat` |
| 6 | `--offered=3Mbps`, `--offered=8Mbps` (link 5 Mbps) |
| 7 | `--fault=0`, `--fault=1`, `--fault=2` |
| 8 | `--offered=5Mbps`, `--offered=40Mbps` (or `--distance=`) |
| 9 | `--speed=0`, `--speed=10` |
| 10 | default (wired + Wi-Fi in one run), optionally vary `--offered` |

## Reading the output
Columns: Tx/Rx packets, Lost, PDR (%) = Rx/Tx x 100, throughput (Mbps), mean delay (ms).
TCP runs also list a reverse flow (the ACKs); ignore its throughput for the data comparison.

## What to explain (per question)
1. Throughput is capped at min(offered, link rate): at 5 Mbps the 8 Mbps load saturates the link
   (loss + queueing delay); at 10 Mbps it all gets through. Delay also falls because
   transmission time = packet size / rate halves.
2. TCP is reliable and congestion-controlled: it adapts to the share left by UDP and retransmits
   losses. UDP sends at a fixed rate regardless of congestion and never retransmits.
3. Two senders (8 Mbps total) fit in 10 Mbps but not in 1 Mbps. At 1 Mbps the router queue
   overflows, so loss is high and each sender gets roughly half of 1 Mbps.
4. DropTail lets the queue fill, so delay stays near (queue length x service time) and drops happen
   only when full (tail drops). RED/CoDel drop earlier, giving lower delay. Judge from your own table.
5. Look for exponential growth (slow start), linear growth (congestion avoidance), and a sharp
   cut when a packet is lost (fast recovery / timeout resets to 1 segment).
6. Below capacity PDR ~ 100%; above capacity PDR ~ link rate / offered rate (e.g. 5/8 = 62.5%).
7. Fault 1: R2 has no route, packets dropped there, Dest receives 0 bytes before `fixAt`. Fault 2:
   wrong next hop sends packets back to R1 (loop, TTL expiry). After the fix, bytes rise.
   Fill the addressing table from the program's printed table; attach the pcaps as evidence.
8. Under light load both stations get what they offer (PDR near 100%). When saturated, they share
   the channel via CSMA/CA, so the aggregate is capped and PDR drops.
9. Throughput holds near the offered rate while close, then the rate adaptation falls to lower
   MCS and finally the link breaks as signal strength decreases with distance (path loss).
10. Typical reasons: Wi-Fi shares a half-duplex medium (contention, backoff, retransmissions,
    headers/ACKs), has a variable PHY rate and signal-dependent loss, whereas the wired link is
    dedicated, full-duplex and error-free.
