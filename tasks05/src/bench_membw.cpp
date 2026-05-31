/**
 * @file bench_membw.cpp
 * @brief Memory bandwidth measurement for Apple M1 Pro (ARM64).
 *
 * Measures sequential read, write, and read+write bandwidth
 * using large arrays that exceed L2/L3 cache sizes.
 *
 * Apple M1 Pro specs (theoretical):
 *   - Memory type:  LPDDR5 unified memory
 *   - Theoretical peak BW: ~200 GB/s (M1 Pro 10-core)
 *
 * Build:
 *   clang++ -O2 -std=c++17 -o bench_membw bench_membw.cpp
 *
 * Run:
 *   ./bench_membw
 */

#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <vector>

static double now_s() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

// Array size: must exceed all cache levels.
// M1 Pro: L1=192KB, L2=12MB per cluster, L3 (SLC) ~24MB
// Use 256 MB to be firmly in DRAM territory.
static constexpr std::size_t ARRAY_BYTES = 256ULL * 1024 * 1024;
static constexpr std::size_t N           = ARRAY_BYTES / sizeof(double);
static constexpr int         REPS        = 8;

static volatile double force_use = 0.0;

// ── kernels ──────────────────────────────────────────────────────────────────

/// Sequential read: accumulate all elements.
static double bw_read(const double* __restrict__ a, std::size_t n) {
    double s = 0.0;
    for (std::size_t i = 0; i < n; ++i) s += a[i];
    force_use = s;
    return s;
}

/// Sequential write: fill with index.
static void bw_write(double* __restrict__ a, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) a[i] = static_cast<double>(i);
}

/// Copy (read + write): classic STREAM copy.
static void bw_copy(double* __restrict__ dst,
                    const double* __restrict__ src,
                    std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) dst[i] = src[i];
}

// ── runner ────────────────────────────────────────────────────────────────────

static double measure_read(const double* a, std::size_t n) {
    double best = 0.0;
    for (int r = 0; r < REPS; ++r) {
        double t0 = now_s();
        bw_read(a, n);
        double dt = now_s() - t0;
        double bw = static_cast<double>(n * sizeof(double)) / dt / 1e9;
        if (bw > best) best = bw;
    }
    return best;
}

static double measure_write(double* a, std::size_t n) {
    double best = 0.0;
    for (int r = 0; r < REPS; ++r) {
        double t0 = now_s();
        bw_write(a, n);
        double dt = now_s() - t0;
        double bw = static_cast<double>(n * sizeof(double)) / dt / 1e9;
        if (bw > best) best = bw;
    }
    return best;
}

static double measure_copy(double* dst, const double* src, std::size_t n) {
    double best = 0.0;
    for (int r = 0; r < REPS; ++r) {
        double t0 = now_s();
        bw_copy(dst, src, n);
        // copy touches 2×n×8 bytes (read + write)
        double dt = now_s() - t0;
        double bw = 2.0 * static_cast<double>(n * sizeof(double)) / dt / 1e9;
        if (bw > best) best = bw;
    }
    return best;
}

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "Memory bandwidth benchmark  |  Apple M1 Pro (ARM64)\n";
    std::cout << "Array size: " << ARRAY_BYTES / (1024 * 1024) << " MB per buffer\n\n";

    std::vector<double> a(N, 1.0), b(N, 0.0);

    double bw_r = measure_read (a.data(), N);
    double bw_w = measure_write(a.data(), N);
    double bw_c = measure_copy (b.data(), a.data(), N);

    std::cout << std::fixed << std::setprecision(1);
    std::cout << std::left << std::setw(30) << "Sequential read"
              << std::right << std::setw(10) << bw_r << "  GB/s\n";
    std::cout << std::left << std::setw(30) << "Sequential write"
              << std::right << std::setw(10) << bw_w << "  GB/s\n";
    std::cout << std::left << std::setw(30) << "Copy (read + write)"
              << std::right << std::setw(10) << bw_c << "  GB/s\n";

    std::cout << "\nTheoretical peak (M1 Pro 10-core): ~200 GB/s\n";
    std::cout << "Use copy BW as the roofline memory ceiling.\n";
    return 0;
}
