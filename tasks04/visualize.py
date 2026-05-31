"""
plot_benchmarks.py
──────────────────
Строит боксплоты по файлу benchmark_results.csv, который генерирует
программа поиска подстрок (main.rs).

Использование:
    python plot_benchmarks.py                        # ищет benchmark_results.csv рядом
    python plot_benchmarks.py path/to/results.csv   # явный путь к файлу
"""

import sys
import pathlib
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

# ── Входной файл ──────────────────────────────────────────────────────────────

csv_path = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "benchmark_results.csv")
if not csv_path.exists():
    sys.exit(f"Файл не найден: {csv_path}")

df = pd.read_csv(csv_path)

# Ожидаемые колонки: run, naive, kmp, bm
required = {"run", "naive", "kmp", "bm"}
if not required.issubset(df.columns):
    sys.exit(f"CSV должен содержать колонки: {required}. Найдено: {set(df.columns)}")

# ── Данные для графика ────────────────────────────────────────────────────────

algo_cols   = ["naive", "kmp", "bm"]
algo_labels = ["Наивный", "КМП", "Бойер-Мур"]
data        = [df[col].values for col in algo_cols]

# Цвета: тёплый/нейтральный/холодный — чтобы легко различать
colors = ["#e07b54", "#5b8dd9", "#62b35e"]

# ── Построение ────────────────────────────────────────────────────────────────

fig, ax = plt.subplots(figsize=(8, 5))

bp = ax.boxplot(
    data,
    labels=algo_labels,
    patch_artist=True,      # закрашенные прямоугольники
    widths=0.45,
    medianprops=dict(color="white", linewidth=2.5),
    whiskerprops=dict(linewidth=1.4),
    capprops=dict(linewidth=1.4),
    flierprops=dict(marker="o", markersize=4, linestyle="none",
                    markeredgewidth=0.8),
)

for patch, color in zip(bp["boxes"], colors):
    patch.set_facecolor(color)
    patch.set_alpha(0.82)

for flier, color in zip(bp["fliers"], colors):
    flier.set(markerfacecolor=color, markeredgecolor=color)

# Подписи средних значений над каждым боксплотом
for i, col_data in enumerate(data, start=1):
    mean_val = col_data.mean()
    ax.text(
        i, col_data.max() * 1.015,
        f"avg: {mean_val:,.0f}",
        ha="center", va="bottom",
        fontsize=8.5, color="#444444",
           )

# ── Оформление ────────────────────────────────────────────────────────────────

ax.set_title("Время поиска подстроки (мкс)", fontsize=13, pad=12)
ax.set_ylabel("Время, мкс", fontsize=10)
ax.set_xlabel("Алгоритм", fontsize=10)

ax.yaxis.set_major_formatter(ticker.FuncFormatter(lambda x, _: f"{x:,.0f}"))
ax.yaxis.grid(True, linestyle="--", alpha=0.5)
ax.set_axisbelow(True)

fig.tight_layout()

# ── Сохранение ────────────────────────────────────────────────────────────────

out_path = csv_path.parent / "boxplot.png"
fig.savefig(out_path, dpi=150)
print(f"График сохранён: {out_path}")

plt.show()
