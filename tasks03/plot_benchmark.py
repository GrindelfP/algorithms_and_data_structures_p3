#!/usr/bin/env python3
"""
Строит боксплот по данным из benchmark_results.csv,
сгенерированного программой trie_task.
"""
import csv
import sys
import statistics

# ── Попытка импортировать matplotlib ──────────────────────────────────────────
try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    HAS_MPL = True
except ImportError:
    HAS_MPL = False

# ── Чтение CSV ────────────────────────────────────────────────────────────────
csv_file = sys.argv[1] if len(sys.argv) > 1 else "benchmark_results.csv"

hash_times, trie_times = [], []

with open(csv_file, newline="") as f:
    reader = csv.DictReader(f)
    for row in reader:
        t = float(row["seconds"])
        if row["method"] == "hash":
            hash_times.append(t)
        else:
            trie_times.append(t)

def fmt(vals):
    lo, hi = min(vals), max(vals)
    med = statistics.median(vals)
    avg = statistics.mean(vals)
    return f"min={lo:.4f}  med={med:.4f}  avg={avg:.4f}  max={hi:.4f}"

speedup = statistics.mean(hash_times) / statistics.mean(trie_times)

print("=== Benchmark summary ===")
print(f"hash ({len(hash_times)} runs): {fmt(hash_times)}")
print(f"trie ({len(trie_times)} runs): {fmt(trie_times)}")
print(f"Среднее ускорение: {speedup:.5f}x")
print()

# ── ASCII-боксплот (всегда) ───────────────────────────────────────────────────
def ascii_boxplot(label, vals, width=50):
    lo, q1, med, q3, hi = (
        min(vals),
        statistics.quantiles(vals, n=4)[0],
        statistics.median(vals),
        statistics.quantiles(vals, n=4)[2],
        max(vals),
    )
    global_lo = min(min(hash_times), min(trie_times))
    global_hi = max(max(hash_times), max(trie_times))
    span = global_hi - global_lo or 1e-9

    def pos(v):
        return int((v - global_lo) / span * (width - 1))

    row = [" "] * width
    # whiskers
    for x in range(pos(lo), pos(hi) + 1):
        row[x] = "-"
    # box
    for x in range(pos(q1), pos(q3) + 1):
        row[x] = "="
    # median
    row[pos(med)] = "|"
    # tails
    row[pos(lo)] = "["
    row[pos(hi)] = "]"

    bar = "".join(row)
    print(f"{label:5s} {bar}  [{lo:.4f} … {hi:.4f}] med={med:.4f}")

print("ASCII boxplot (time in seconds, lower = faster):")
print(f"      {'|':<50}  range")
ascii_boxplot("hash", hash_times)
ascii_boxplot("trie", trie_times)
print(f"      {min(hash_times+trie_times):.4f}" + " " * 38 +
      f"{max(hash_times+trie_times):.4f}")
print()

# ── matplotlib-боксплот (если доступен) ──────────────────────────────────────
if HAS_MPL:
    fig, ax = plt.subplots(figsize=(8, 5))

    data   = [hash_times, trie_times]
    labels = ["HashMap\n(std)", "Trie\n(arena)"]
    colors = ["#e07b54", "#5b9bd5"]

    bp = ax.boxplot(data, patch_artist=True, widths=0.45,
                    medianprops=dict(color="black", linewidth=2))
    for patch, color in zip(bp["boxes"], colors):
        patch.set_facecolor(color)
        patch.set_alpha(0.75)

    # рассыпка точек
    for i, vals in enumerate(data, start=1):
        xs = [i + (j % 3 - 1) * 0.06 for j in range(len(vals))]
        ax.scatter(xs, vals, zorder=5, s=30, color="black", alpha=0.6)

    ax.set_title(
        f"Word-count dictionary: HashMap vs Trie\n"
        f"Среднее ускорение Trie: {speedup:.4f}×  "
        f"(n={len(hash_times)} runs)",
        fontsize=13,
    )
    ax.set_ylabel("Wall-clock time, seconds")
    ax.set_xticks([1, 2])
    ax.set_xticklabels(labels, fontsize=12)
    ax.grid(axis="y", linestyle="--", alpha=0.5)

    out_file = "benchmark_boxplot.png"
    fig.tight_layout()
    fig.savefig(out_file, dpi=150)
    print(f"Boxplot saved to '{out_file}'")
else:
    print("matplotlib not found — skipped PNG generation.")
    print("Install with:  pip install matplotlib")
