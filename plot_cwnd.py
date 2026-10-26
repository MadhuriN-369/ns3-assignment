# Usage: python3 plot_cwnd.py q5-cwnd.dat   (needs matplotlib)
import sys
import matplotlib.pyplot as plt
t, w = [], []
for line in open(sys.argv[1] if len(sys.argv) > 1 else "q5-cwnd.dat"):
    a, b = line.split()
    t.append(float(a)); w.append(int(b))
plt.step(t, w, where="post")
plt.xlabel("Time (s)"); plt.ylabel("cwnd (bytes)"); plt.title("TCP congestion window")
plt.grid(True); plt.savefig("q5-cwnd.png", dpi=150); print("saved q5-cwnd.png")
