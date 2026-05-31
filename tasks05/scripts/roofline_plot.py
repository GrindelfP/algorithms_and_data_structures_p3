#!/usr/bin/env python3
"""
roofline_plot.py
────────────────
Builds a Cache-Aware Roofline chart for the edge-cost kernel
benchmarked on Apple M1 Pro.

Usage:
    python3 roofline_plot.py

Edit the MEASURED section below with your actual numbers after running
bench_membw and bench_edge_cost.
"""

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.lines import Line2D

# ── MEASURED — fill these in after running your benchmarks ───────────────────

# Memory bandwidth ceilings (GB/s) — from bench_membw output
BW_DRAM_COPY   = 8.4    # replace with your copy BW result
BW_DRAM_READ   = 61.5   # replace with your read BW result

# L2 and L1 cache bandwidth estimates for M1 Pro (typical, hard to measure
# directly without perf counters; use these conservative estimates or adjust).
BW_L2 = 300.0   # GB/s  (M1 Pro L2 ~500 GB/s peak, use conservative)
BW_L1 = 800.0   # GB/s  (M1 Pro L1 ~1 TB/s peak, use conservative)

# Scalar peak compute (GFLOP/s) — single-core, double precision, no SIMD
# M1 Pro: 1 FMA/cycle × 2 FLOP × ~3.2 GHz ≈ 6.4 GFLOP/s scalar DP
PEAK_SCALAR_DP = 6.4

# SIMD peak (NEON, 2×FMA, 2 doubles/lane) ≈ 4× scalar
PEAK_SIMD_DP = 25.6

# ── KERNEL DATA POINTS — from bench_edge_cost output ────────────────────────
# Format: (label, AI [FLOP/byte], measured_GFLOPS)
# Fill in your actual GFLOP/s values from the benchmark table.

kernels = [
    ("AoS + sqrt\n(baseline)", 0.78, 23.746),  
    ("AoS + no-sqrt",          0.16, 5.330),   
    ("SoA + sqrt",             1.56, 26.434),  
    ("SoA + no-sqrt\n(best)",  0.31, 5.365),   
]

# ── PLOT ─────────────────────────────────────────────────────────────────────

fig, ax = plt.subplots(figsize=(10, 6))
fig.patch.set_facecolor("#0f0f14")
ax.set_facecolor("#0f0f14")

AI_range = np.logspace(-2, 2, 500)   # 0.01 .. 100 FLOP/byte

COLORS = {
    "dram_copy": "#4fc3f7",
    "dram_read": "#81d4fa",
    "l2":        "#b39ddb",
    "l1":        "#ef9a9a",
    "scalar":    "#a5d6a7",
    "simd":      "#ffcc80",
}

def roofline(bw_gb_s, peak_gflops, ai):
    """Ridge at intersection of memory roof and compute roof."""
    return np.minimum(bw_gb_s * ai, peak_gflops)

# Draw memory roofs
for bw, label, color, ls in [
    (BW_DRAM_COPY, f"DRAM copy  {BW_DRAM_COPY:.0f} GB/s", COLORS["dram_copy"], "-"),
    (BW_DRAM_READ, f"DRAM read  {BW_DRAM_READ:.0f} GB/s",  COLORS["dram_read"], "--"),
    (BW_L2,        f"L2 cache   {BW_L2:.0f} GB/s",         COLORS["l2"],        "-."),
    (BW_L1,        f"L1 cache   {BW_L1:.0f} GB/s",         COLORS["l1"],        ":"),
]:
    y = roofline(bw, PEAK_SIMD_DP, AI_range)
    ax.loglog(AI_range, y, color=color, lw=1.5, ls=ls, label=label, alpha=0.85)

# Draw compute roofs (horizontal)
ax.axhline(PEAK_SCALAR_DP, color=COLORS["scalar"], lw=1.5, ls="--",
           label=f"Scalar DP peak  {PEAK_SCALAR_DP} GFLOP/s", alpha=0.85)
ax.axhline(PEAK_SIMD_DP,   color=COLORS["simd"],   lw=2.0, ls="-",
           label=f"SIMD DP peak  {PEAK_SIMD_DP} GFLOP/s",   alpha=0.85)

# Draw kernel points
POINT_COLORS = ["#ff6b6b", "#ffd93d", "#6bcb77", "#4d96ff"]
for (label, ai, gflops), color in zip(kernels, POINT_COLORS):
    if gflops is None:
        # Draw a vertical guideline so you can see where it falls
        ax.axvline(ai, color=color, lw=1, ls=":", alpha=0.5)
        ax.text(ai * 1.05, 0.015, label, color=color, fontsize=7,
                va="bottom", rotation=90)
    else:
        ax.scatter([ai], [gflops], color=color, s=80, zorder=5)
        ax.annotate(label, (ai, gflops),
                    textcoords="offset points", xytext=(6, 4),
                    color=color, fontsize=8)

# Formatting
ax.set_xlabel("Arithmetic Intensity  [FLOP / byte]",
              color="#cccccc", fontsize=11)
ax.set_ylabel("Performance  [GFLOP/s]",
              color="#cccccc", fontsize=11)
ax.set_title("Cache-Aware Roofline — Edge Cost Kernel\nApple M1 Pro · Double Precision",
             color="#eeeeee", fontsize=13, pad=14)

ax.set_xlim(0.05, 100)
ax.set_ylim(0.01, PEAK_SIMD_DP * 3)

ax.tick_params(colors="#aaaaaa")
for spine in ax.spines.values():
    spine.set_edgecolor("#333344")

ax.grid(True, which="both", color="#222233", lw=0.6, alpha=0.8)
ax.legend(loc="upper left", fontsize=8,
          facecolor="#1a1a24", edgecolor="#444455",
          labelcolor="#cccccc")

plt.tight_layout()
plt.savefig("roofline.png", dpi=150, bbox_inches="tight",
            facecolor=fig.get_facecolor())
print("Saved roofline.png")
plt.show()
