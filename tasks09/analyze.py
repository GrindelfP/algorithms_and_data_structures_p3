#!/usr/bin/env python3
"""
Анализ результатов многопоточности.
Читает results.csv, выводит таблицу и строит графики ускорения и эффективности.
"""

import csv
import os
import sys
import math
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np

CSV_FILE = "results.csv"
TABLE_FILE = "timing_table.txt"
SPEEDUP_FILE = "speedup.png"
EFFICIENCY_FILE = "efficiency.png"


def load_csv(path: str) -> list[dict]:
    with open(path, newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def to_int(s):
    try:
        return int(s)
    except ValueError:
        return None


def build_table(rows: list[dict]) -> str:
    methods = ["block", "queue", "pool"]
    header = (
        f"{'Потоков':>8}  "
        f"{'single (мс)':>12}  "
        + "  ".join(f"{m+' (мс)':>12}" for m in methods)
    )
    sep = "-" * len(header)

    lines = [
        "ТАБЛИЦА ВРЕМЕНИ ВЫПОЛНЕНИЯ",
        "=" * len(header),
        header,
        sep,
    ]

    for row in rows:
        line = (
            f"{row['nthreads']:>8}  "
            f"{row['single_ms']:>12}  "
            + "  ".join(f"{row[m+'_ms']:>12}" for m in methods)
        )
        lines.append(line)

    lines.append("")
    lines.append("УСКОРЕНИЕ  (speedup = T_single / T_parallel)")
    lines.append("=" * len(header))
    lines.append(header.replace("(мс)", "    ").replace("single", "single   "))

    single_times = {row["nthreads"]: to_int(row["single_ms"]) for row in rows}
    t_single = to_int(rows[0]["single_ms"])

    lines.append(
        f"{'Потоков':>8}  "
        f"{'1.000':>12}  "
        + "  ".join(f"{'speedup':>12}" for _ in methods)
    )
    lines.append(sep)

    for row in rows:
        nt = row["nthreads"]
        parts = [f"{nt:>8}  {'1.000':>12}  "]
        for m in methods:
            t = to_int(row[m + "_ms"])
            sp = t_single / t if t and t > 0 else float("nan")
            parts.append(f"{sp:>12.3f}  ")
        lines.append("".join(parts))

    lines.append("")
    lines.append("ЭФФЕКТИВНОСТЬ  (efficiency = speedup / N_threads)")
    lines.append("=" * len(header))
    lines.append(sep)

    for row in rows:
        nt_val = to_int(row["nthreads"])
        parts = [f"{row['nthreads']:>8}  {'1.000':>12}  "]
        for m in methods:
            t = to_int(row[m + "_ms"])
            sp = t_single / t if t and t > 0 else float("nan")
            ef = sp / nt_val if nt_val else float("nan")
            parts.append(f"{ef:>12.3f}  ")
        lines.append("".join(parts))

    return "\n".join(lines)


# ── цвета и маркеры ──────────────────────────────────────────────────────────
STYLES = {
    "block": dict(color="#4e79c4", marker="o", linestyle="-",  label="block"),
    "queue": dict(color="#c44e4e", marker="^", linestyle="-",  label="queue"),
    "pool":  dict(color="#4ec47a", marker="s", linestyle="--", label="pool"),
}


def plot_speedup(rows: list[dict], out_path: str):
    t_single = to_int(rows[0]["single_ms"])
    nthreads = [to_int(r["nthreads"]) for r in rows]
    ideal = nthreads

    fig, ax = plt.subplots(figsize=(7, 5))
    ax.plot(nthreads, ideal, color="lightgray", linestyle="--",
            linewidth=1.2, label="ideal", zorder=1)

    for m in ["block", "queue", "pool"]:
        speedups = []
        for row in rows:
            t = to_int(row[m + "_ms"])
            speedups.append(t_single / t if t and t > 0 else float("nan"))
        ax.plot(nthreads, speedups, **STYLES[m], linewidth=2, markersize=7, zorder=3)

    ax.set_xlabel("число потоков", fontsize=12)
    ax.set_ylabel("ускорение", fontsize=12)
    ax.set_title("Ускорение (Speedup)", fontsize=14)
    ax.legend(fontsize=10)
    ax.xaxis.set_major_locator(ticker.MaxNLocator(integer=True))
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)
    print(f"[OK] График ускорения сохранён: {out_path}")


def plot_efficiency(rows: list[dict], out_path: str):
    t_single = to_int(rows[0]["single_ms"])
    nthreads = [to_int(r["nthreads"]) for r in rows]

    fig, ax = plt.subplots(figsize=(7, 5))
    ax.axhline(1.0, color="lightgray", linestyle="--", linewidth=1.2,
               label="ideal", zorder=1)

    for m in ["block", "queue", "pool"]:
        efficiencies = []
        for row in rows:
            nt = to_int(row["nthreads"])
            t = to_int(row[m + "_ms"])
            sp = t_single / t if t and t > 0 else float("nan")
            efficiencies.append(sp / nt if nt else float("nan"))
        ax.plot(nthreads, efficiencies, **STYLES[m], linewidth=2, markersize=7, zorder=3)

    ax.set_xlabel("число потоков", fontsize=12)
    ax.set_ylabel("эффективность", fontsize=12)
    ax.set_title("Эффективность (Efficiency)", fontsize=14)
    ax.legend(fontsize=10)
    ax.xaxis.set_major_locator(ticker.MaxNLocator(integer=True))
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)
    print(f"[OK] График эффективности сохранён: {out_path}")


def main():
    if not os.path.exists(CSV_FILE):
        print(f"[ERROR] Файл {CSV_FILE} не найден.")
        print("Сначала запустите Kotlin-программу, чтобы сгенерировать results.csv")
        sys.exit(1)

    rows = load_csv(CSV_FILE)

    table_text = build_table(rows)
    with open(TABLE_FILE, "w", encoding="utf-8") as f:
        f.write(table_text)
    print(f"[OK] Таблица сохранена: {TABLE_FILE}")
    print()
    print(table_text)
    print()

    plot_speedup(rows, SPEEDUP_FILE)
    plot_efficiency(rows, EFFICIENCY_FILE)


if __name__ == "__main__":
    main()

