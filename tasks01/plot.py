#!/usr/bin/env python3
"""
plot.py — Build graphs for task 1.2 (sorting a linked list).

Reads:  results.csv   (produced by ./bench)
Writes: plots/*.png

Charts produced
───────────────
1.  Comparisons vs n  — one subplot per input type, both sorts overlaid
2.  Pointer swaps vs n — same layout
3.  Time (ms) vs n     — same layout
4.  All-in-one summary — 3×4 grid (metric × input_type)
5.  Asymptotic overlay — compares O(n²) and O(n log n) curves with data

Dependencies: pandas, matplotlib  (pip install pandas matplotlib)
"""

import os
import sys
import math

import pandas as pd
import matplotlib
matplotlib.use("Agg")          # no display required
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

# ── paths ──────────────────────────────────────────────────────────────────
CSV_PATH   = "results.csv"
PLOTS_DIR  = "plots"

os.makedirs(PLOTS_DIR, exist_ok=True)

# ── load data ───────────────────────────────────────────────────────────────
if not os.path.exists(CSV_PATH):
    sys.exit(f"ERROR: {CSV_PATH} not found. Run ./bench first.")

df = pd.read_csv(CSV_PATH)

if df["status"].ne("ok").any():
    bad = df[df["status"] != "ok"]
    print(f"WARNING: {len(bad)} failed sort(s) detected!")
    print(bad[["sort_type","input_type","n"]].drop_duplicates().to_string(index=False))

# Time in milliseconds for readability
df["time_ms"] = df["time_us"] / 1e3

# Aggregate: mean over RUNS independent runs
agg = df.groupby(["sort_type", "input_type", "n"], as_index=False).agg(
    cmp_mean   = ("comparisons", "mean"),
    cmp_std    = ("comparisons", "std"),
    swap_mean  = ("ptr_swaps",   "mean"),
    swap_std   = ("ptr_swaps",   "std"),
    time_mean  = ("time_ms",     "mean"),
    time_std   = ("time_ms",     "std"),
)

# ── colour / style map ──────────────────────────────────────────────────────
STYLE = {
    "insertion": dict(color="#E63946", marker="o", ls="-",  lw=1.8),
    "merge":     dict(color="#457B9D", marker="s", ls="--", lw=1.8),
}

INPUT_LABELS = {
    "random":  "Random",
    "sorted":  "Already sorted",
    "reverse": "Reverse order",
    "nearly":  "Nearly sorted",
}
INPUT_ORDER = ["random", "sorted", "reverse", "nearly"]

METRIC_INFO = {
    "cmp_mean":  ("Comparisons",         "cmp_std"),
    "swap_mean": ("Pointer assignments",  "swap_std"),
    "time_mean": ("Time (ms)",            "time_std"),
}

# ── helper ──────────────────────────────────────────────────────────────────
def savefig(fig, name):
    path = os.path.join(PLOTS_DIR, name)
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  saved {path}")


def plot_metric_grid(metric_key, ylabel, std_key, filename):
    """One figure: 1 row × 4 subplots, one per input type."""
    fig, axes = plt.subplots(1, 4, figsize=(16, 4), sharey=False)
    fig.suptitle(f"{ylabel} vs list size  (Variant 12)", fontsize=13, y=1.02)

    for ax, itype in zip(axes, INPUT_ORDER):
        for stype, st in STYLE.items():
            sub = agg[(agg.sort_type == stype) & (agg.input_type == itype)]
            ax.errorbar(
                sub["n"], sub[metric_key], yerr=sub[std_key],
                label=stype.capitalize(),
                color=st["color"], marker=st["marker"],
                ls=st["ls"], lw=st["lw"],
                capsize=3, elinewidth=0.8, alpha=0.9,
            )
        ax.set_title(INPUT_LABELS[itype], fontsize=10)
        ax.set_xlabel("n", fontsize=9)
        ax.yaxis.set_major_formatter(ticker.FuncFormatter(
            lambda v, _: f"{v:,.0f}" if v >= 1000 else f"{v:.3g}"
        ))
        ax.grid(True, alpha=0.3)
        ax.legend(fontsize=8)

    axes[0].set_ylabel(ylabel, fontsize=9)
    fig.tight_layout()
    savefig(fig, filename)


# ── charts 1-3: one metric per figure ───────────────────────────────────────
print("Generating per-metric charts …")
plot_metric_grid("cmp_mean",  "Comparisons",        "cmp_std",  "1_comparisons.png")
plot_metric_grid("swap_mean", "Pointer assignments", "swap_std", "2_pointer_swaps.png")
plot_metric_grid("time_mean", "Time (ms)",           "time_std", "3_time_ms.png")

# ── chart 4: 3×4 summary grid ───────────────────────────────────────────────
print("Generating summary grid …")
fig, axes = plt.subplots(3, 4, figsize=(18, 12))
fig.suptitle("Sorting metrics — Variant 12  (mean ± std over 10 runs)", fontsize=14)

metrics = [
    ("cmp_mean",  "cmp_std",  "Comparisons"),
    ("swap_mean", "swap_std", "Pointer assignments"),
    ("time_mean", "time_std", "Time (ms)"),
]

for row, (mk, sk, mlabel) in enumerate(metrics):
    for col, itype in enumerate(INPUT_ORDER):
        ax = axes[row][col]
        for stype, st in STYLE.items():
            sub = agg[(agg.sort_type == stype) & (agg.input_type == itype)]
            ax.errorbar(
                sub["n"], sub[mk], yerr=sub[sk],
                label=stype.capitalize(),
                color=st["color"], marker=st["marker"],
                ls=st["ls"], lw=st["lw"],
                capsize=3, elinewidth=0.8, alpha=0.9,
            )
        if row == 0:
            ax.set_title(INPUT_LABELS[itype], fontsize=10)
        if col == 0:
            ax.set_ylabel(mlabel, fontsize=9)
        if row == 2:
            ax.set_xlabel("n", fontsize=9)
        ax.grid(True, alpha=0.3)
        ax.legend(fontsize=7)

fig.tight_layout(rect=[0, 0, 1, 0.97])
savefig(fig, "4_summary_grid.png")

# ── chart 5: asymptotic overlay ─────────────────────────────────────────────
print("Generating asymptotic overlay …")
fig, axes = plt.subplots(1, 2, figsize=(12, 5))
fig.suptitle("Asymptotic comparison: measured vs theoretical  (random input)", fontsize=12)

sub_ins = agg[(agg.sort_type == "insertion") & (agg.input_type == "random")].copy()
sub_mrg = agg[(agg.sort_type == "merge")     & (agg.input_type == "random")].copy()

ns = sub_ins["n"].values

# Normalise theoretical curves to match data at first point
def norm_curve(curve, data_col, sub):
    factor = sub[data_col].iloc[0] / curve[0] if curve[0] != 0 else 1
    return curve * factor

n2      = ns ** 2
n_logn  = ns * (ns > 0) * [math.log2(x) if x > 1 else 1 for x in ns]

for ax, (metric_key, std_key, ylabel) in zip(axes, [
    ("cmp_mean", "cmp_std", "Comparisons"),
    ("swap_mean","swap_std","Pointer assignments"),
]):
    # Measured data
    ax.errorbar(ns, sub_ins[metric_key], yerr=sub_ins[std_key],
                label="Insertion (measured)", capsize=3,
                **{k: v for k, v in STYLE["insertion"].items()})
    ax.errorbar(ns, sub_mrg[metric_key], yerr=sub_mrg[std_key],
                label="Merge (measured)", capsize=3,
                **{k: v for k, v in STYLE["merge"].items()})

    # Theoretical curves
    ins_theory = norm_curve(n2.astype(float),    metric_key, sub_ins)
    mrg_theory = norm_curve(n_logn.astype(float), metric_key, sub_mrg)

    ax.plot(ns, ins_theory, color="#E63946", ls=":", lw=1.2, alpha=0.6, label="O(n²)")
    ax.plot(ns, mrg_theory, color="#457B9D", ls=":", lw=1.2, alpha=0.6, label="O(n log n)")

    ax.set_xlabel("n", fontsize=10)
    ax.set_ylabel(ylabel, fontsize=10)
    ax.set_title(f"{ylabel} (random input)", fontsize=10)
    ax.legend(fontsize=8)
    ax.grid(True, alpha=0.3)

fig.tight_layout()
savefig(fig, "5_asymptotic.png")

# ── done ────────────────────────────────────────────────────────────────────
print(f"\nAll charts written to ./{PLOTS_DIR}/")
