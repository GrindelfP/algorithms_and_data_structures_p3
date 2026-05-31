/**
 * @file bench_edge_cost.cpp
 * @brief Roofline benchmark: edge cost computation for Minimum Weight Triangulation.
 *
 * Isolates the innermost loop of the greedy/quasi-greedy triangulation:
 * for every pair of points, compute the Euclidean distance (potential edge weight).
 *
 * Four variants are measured:
 *   1. AoS  + sqrt   — baseline, array of structs, full sqrt
 *   2. AoS  + no-sqrt — squared distances (valid for pure comparisons)
 *   3. SoA  + sqrt   — separate x[] and y[] arrays (better cache line use)
 *   4. SoA  + no-sqrt — best case: SoA layout + avoid sqrt
 *
 * Build:
 *   clang++ -O2 -std=c++17 -o bench bench_edge_cost.cpp
 *   clang++ -O3 -std=c++17 -o bench_O3 bench_edge_cost.cpp
 *
 * Run:
 *   ./bench [N]        (default N = 4096)
 */

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

// ── helpers ──────────────────────────────────────────────────────────────────

static double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(
               steady_clock::now().time_since_epoch())
        .count();
}

// Prevent the compiler from optimizing away the result of a computation.
// Works by writing through a volatile pointer.
static volatile double sink = 0.0;
static void do_not_optimize(double v) { sink = v; }

// ── data layouts ─────────────────────────────────────────────────────────────

struct Point2D {   // Array-of-Structs element
    double x, y;
};

struct SoAPoints { // Structure-of-Arrays
    std::vector<double> x, y;
    explicit SoAPoints(std::size_t n) : x(n), y(n) {}
};

// ── benchmark kernels ─────────────────────────────────────────────────────────

/**
 * @brief Variant 1 — AoS layout, full sqrt.
 *
 * Arithmetic intensity (theoretical):
 *   Operations per pair: dx=1sub, dy=1sub, dx²=1mul, dy²=1mul,
 *                        sum=1add, sqrt=~20ops ≈ 25 FLOP
 *   Memory per pair: 2 × sizeof(Point2D) = 32 bytes (cold cache)
 *   AI ≈ 25 / 32 ≈ 0.78 FLOP/byte
 *
 * In practice with N²/2 pairs and N×16 bytes total data,
 * each point is reused N times → effective AI rises with N.
 */
static double kernel_aos_sqrt(const std::vector<Point2D>& pts, std::size_t n) {
    double total = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            double dx = pts[i].x - pts[j].x;
            double dy = pts[i].y - pts[j].y;
            total += std::sqrt(dx * dx + dy * dy);
        }
    }
    return total;
}

/**
 * @brief Variant 2 — AoS layout, squared distance (no sqrt).
 *
 * Drops the sqrt ≈ 20 ops → ~5 FLOP per pair.
 * Same memory access pattern as variant 1.
 * AI ≈ 5 / 32 ≈ 0.16 FLOP/byte  → more memory-bound.
 */
static double kernel_aos_nosqrt(const std::vector<Point2D>& pts, std::size_t n) {
    double total = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            double dx = pts[i].x - pts[j].x;
            double dy = pts[i].y - pts[j].y;
            total += dx * dx + dy * dy;
        }
    }
    return total;
}

/**
 * @brief Variant 3 — SoA layout, full sqrt.
 *
 * x[] and y[] are contiguous. The inner loop over j reads
 * pts.x[j] and pts.y[j] sequentially → better prefetch / SIMD potential.
 * Same FLOP count as variant 1, but lower effective memory traffic
 * because only the coordinates actually needed are loaded.
 */
static double kernel_soa_sqrt(const SoAPoints& pts, std::size_t n) {
    double total = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        double xi = pts.x[i], yi = pts.y[i];
        for (std::size_t j = i + 1; j < n; ++j) {
            double dx = xi - pts.x[j];
            double dy = yi - pts.y[j];
            total += std::sqrt(dx * dx + dy * dy);
        }
    }
    return total;
}

/**
 * @brief Variant 4 — SoA layout, no sqrt (best-case).
 *
 * Combines improved layout with reduced arithmetic.
 * Represents the upper bound of achievable throughput for this kernel.
 */
static double kernel_soa_nosqrt(const SoAPoints& pts, std::size_t n) {
    double total = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        double xi = pts.x[i], yi = pts.y[i];
        for (std::size_t j = i + 1; j < n; ++j) {
            double dx = xi - pts.x[j];
            double dy = yi - pts.y[j];
            total += dx * dx + dy * dy;
        }
    }
    return total;
}

// ── runner ───────────────────────────────────────────────────────────────────

struct Result {
    const char* name;
    double time_ms;
    double gflops;
    double gb_s;       // effective memory bandwidth (lower bound)
    double ai;         // arithmetic intensity FLOP/byte
};

template<typename Fn>
static Result run(const char* name,
                  Fn fn,
                  std::size_t n,
                  double flops_per_pair,
                  double bytes_per_pair) {
    // warm-up
    do_not_optimize(fn());

    constexpr int REPS = 5;
    double best_ms = 1e18;
    for (int r = 0; r < REPS; ++r) {
        double t0 = now_ms();
        double v  = fn();
        double t1 = now_ms();
        do_not_optimize(v);
        best_ms = std::min(best_ms, t1 - t0);
    }

    double pairs    = static_cast<double>(n) * (n - 1) / 2.0;
    double total_fl = pairs * flops_per_pair;
    double total_by = pairs * bytes_per_pair;

    Result r;
    r.name     = name;
    r.time_ms  = best_ms;
    r.gflops   = total_fl / (best_ms * 1e-3) / 1e9;
    r.gb_s     = total_by / (best_ms * 1e-3) / 1e9;
    r.ai       = flops_per_pair / bytes_per_pair;
    return r;
}

// ── main ─────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    std::size_t N = 4096;
    if (argc > 1) N = static_cast<std::size_t>(std::atol(argv[1]));

    std::cout << "Edge-cost kernel benchmark  |  N = " << N
              << "  |  pairs = " << N * (N - 1) / 2 << "\n\n";

    // ── generate random points ──
    std::mt19937_64 rng(42);
    std::uniform_real_distribution<double> dist(0.0, 1000.0);

    std::vector<Point2D> aos(N);
    SoAPoints            soa(N);
    for (std::size_t i = 0; i < N; ++i) {
        aos[i].x = soa.x[i] = dist(rng);
        aos[i].y = soa.y[i] = dist(rng);
    }

    // ── FLOP counts ──
    // sqrt variant:    2 sub + 2 mul + 1 add + 1 sqrt ≈ 6 "cheap" + sqrt
    // We model sqrt as 20 equivalent FLOPs (conservative for ARM NEON/VFP).
    // no-sqrt variant: 2 sub + 2 mul + 1 add = 5 FLOPs
    const double FLOP_SQRT   = 25.0;
    const double FLOP_NOSQRT =  5.0;

    // Memory: each pair reads 2 points.
    // AoS: Point2D = 16 bytes → 2 × 16 = 32 bytes/pair
    // SoA: reads x[i],y[i] (cached after first inner loop) + x[j],y[j]
    //      effectively 2 doubles = 16 bytes/pair for the j-loop hot path.
    const double BYTES_AOS = 32.0;
    const double BYTES_SOA = 16.0;

    // ── bind lambdas ──
    auto f1 = [&]() { return kernel_aos_sqrt  (aos, N); };
    auto f2 = [&]() { return kernel_aos_nosqrt(aos, N); };
    auto f3 = [&]() { return kernel_soa_sqrt  (soa, N); };
    auto f4 = [&]() { return kernel_soa_nosqrt(soa, N); };

    std::vector<Result> results = {
        run("AoS + sqrt   (baseline)", [&](){ return f1(); }, N, FLOP_SQRT,   BYTES_AOS),
        run("AoS + no-sqrt          ", [&](){ return f2(); }, N, FLOP_NOSQRT, BYTES_AOS),
        run("SoA + sqrt             ", [&](){ return f3(); }, N, FLOP_SQRT,   BYTES_SOA),
        run("SoA + no-sqrt (best)   ", [&](){ return f4(); }, N, FLOP_NOSQRT, BYTES_SOA),
    };

    // ── print table ──
    std::cout << std::left
              << std::setw(30) << "Variant"
              << std::right
              << std::setw(12) << "Time (ms)"
              << std::setw(12) << "GFLOP/s"
              << std::setw(14) << "BW (GB/s)"
              << std::setw(14) << "AI (F/B)"
              << "\n";
    std::cout << std::string(82, '-') << "\n";

    for (const auto& r : results) {
        std::cout << std::left  << std::setw(30) << r.name
                  << std::right << std::fixed << std::setprecision(3)
                  << std::setw(12) << r.time_ms
                  << std::setw(12) << r.gflops
                  << std::setw(14) << r.gb_s
                  << std::setw(14) << r.ai
                  << "\n";
    }

    std::cout << "\nNote: AI = arithmetic intensity (FLOP/byte).\n"
              << "      BW shown is the lower-bound effective bandwidth.\n";
    return 0;
}
