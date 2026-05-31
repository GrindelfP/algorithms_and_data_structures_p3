#!/bin/bash
set -e

CC="gcc"
STD="-std=c89"
WARN="-Wall -Wextra -pedantic"
OPT="-O2"

echo "=== Building ==="

# Original list demo (task 1.1)
$CC $STD $WARN -o list main.c list.c
echo "  list        (task 1.1 demo)"

# Sort demo (char list, task 1.2)
$CC $STD $WARN -o sort_demo sort_demo.c list.c list_int.c sort.c
echo "  sort_demo   (char sort demo)"

# Benchmark (int list, task 1.2)
$CC $STD $WARN $OPT -o bench bench.c list.c list_int.c sort.c
echo "  bench       (int sort benchmark)"

echo ""
echo "=== Task 1.1 demo ==="
./list

echo ""
echo "=== Task 1.2 sort demo (char) ==="
./sort_demo

echo ""
echo "=== Benchmark (int, writing results.csv) ==="
./bench > results.csv
echo "  Done — $(wc -l < results.csv) rows written to results.csv"

echo ""
echo "=== Plotting ==="
python3 plot.py
echo "  Graphs saved to plots/"
