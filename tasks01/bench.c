/*
 * bench.c — Benchmark for insertion sort and merge sort (Variant 12).
 *
 * Outputs CSV rows to stdout:
 *   sort_type, input_type, n, run, comparisons, ptr_swaps, time_us
 *
 * Compile (benchmark uses POSIX gettimeofday, so -std=gnu89 or -D_POSIX_C_SOURCE):
 *   cc -std=c89 -D_POSIX_C_SOURCE=200112L -Wall -Wextra -O2 \
 *      -o bench bench.c list_int.c sort.c list.c
 *
 * Usage:
 *   ./bench > results.csv
 */

#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "list_int.h"
#include "sort.h"

/* ---- configuration ----------------------------------------------------- */

#define RUNS        10          /* independent runs per (sort, input, n)   */
#define N_STEPS     12          /* how many sizes to test                   */
#define N_START     100         /* smallest list size                       */
#define N_STEP      500         /* increment between sizes                  */
#define NEAR_SORT_PCT 7         /* % of swaps for "nearly sorted" input     */

/* ---- portable microsecond timer --------------------------------------- */
/* Returns microseconds as a double to avoid long long in C89.             */

static double now_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1e6 + (double)tv.tv_usec;
}

/* ---- PRNG (simple LCG — no rand() to keep results reproducible) ------- */

static unsigned long rng_state = 12345UL;

static unsigned long lcg_next(void) {
    rng_state = rng_state * 1664525UL + 1013904223UL;
    return rng_state;
}

static void rng_seed(unsigned long seed) {
    rng_state = seed;
}

/* ---- list filling helpers --------------------------------------------- */

/* Fill list with n random integers in [0, 32767]. */
static void fill_random(struct NodeInt **p, int n) {
    int i;
    for (i = 0; i < n; ++i)
        push_back_int(p, (int)(lcg_next() & 0x7FFF));
}

/* Fill list with n integers in ascending order 0,1,...,n-1. */
static void fill_sorted(struct NodeInt **p, int n) {
    int i;
    for (i = 0; i < n; ++i)
        push_back_int(p, i);
}

/* Fill list with n integers in descending order n-1,...,1,0. */
static void fill_reverse(struct NodeInt **p, int n) {
    int i;
    for (i = n - 1; i >= 0; --i)
        push_back_int(p, i);
}

/*
 * Fill list with n integers in ascending order, then randomly swap
 * NEAR_SORT_PCT % of adjacent pairs.
 */
static void fill_nearly(struct NodeInt **p, int n) {
    struct NodeInt *s, *cur;
    int swaps, i;
    int tmp;

    fill_sorted(p, n);

    swaps = (int)((long)n * NEAR_SORT_PCT / 100);
    if (swaps < 1) swaps = 1;

    s = (*p)->next; /* sentinel */

    for (i = 0; i < swaps; ++i) {
        int pos = (int)(lcg_next() % (unsigned long)(n - 1));
        cur = s->next;
        while (pos-- > 0) cur = cur->next;
        /* swap data of cur and cur->next */
        if (cur->next != s) {
            tmp             = cur->data;
            cur->data       = cur->next->data;
            cur->next->data = tmp;
        }
    }
}

/* ---- check that the list is sorted ------------------------------------ */

static int is_sorted_int(const struct NodeInt *p) {
    const struct NodeInt *s   = p->next; /* sentinel */
    const struct NodeInt *cur = s->next;
    while (cur != s && cur->next != s) {
        if (cur->data > cur->next->data) return 0;
        cur = cur->next;
    }
    return 1;
}

/* ---- run one benchmark scenario --------------------------------------- */

typedef void (*SortFn)(struct NodeInt **, SortMetrics *);

static void run_scenario(
    const char *sort_name,
    const char *input_name,
    SortFn      sort_fn,
    int         n,
    int         run,
    unsigned long seed)
{
    struct NodeInt *p = NULL;
    SortMetrics m;
    double t0, t1;
    int ok;

    rng_seed(seed);

    create_list_int(&p);

    if      (strcmp(input_name, "random")  == 0) fill_random(&p, n);
    else if (strcmp(input_name, "sorted")  == 0) fill_sorted(&p, n);
    else if (strcmp(input_name, "reverse") == 0) fill_reverse(&p, n);
    else                                          fill_nearly(&p, n);

    t0 = now_us();
    sort_fn(&p, &m);
    t1 = now_us();

    ok = is_sorted_int(p);

    /* time_us stored as integer microseconds for clean CSV */
    printf("%s,%s,%d,%d,%ld,%ld,%ld,%s\n",
           sort_name, input_name, n, run,
           m.comparisons, m.ptr_swaps,
           (long)(t1 - t0),
           ok ? "ok" : "FAIL");

    remove_list_int(&p);
}

/* ---- main ------------------------------------------------------------- */

int main(void) {
    int step, run, n;
    unsigned long base_seed = 99991UL;

    const char *input_types[] = { "random", "sorted", "reverse", "nearly" };
    int n_inputs = 4;

    /* CSV header */
    printf("sort_type,input_type,n,run,comparisons,ptr_swaps,time_us,status\n");

    for (step = 0; step < N_STEPS; ++step) {
        int in_idx;
        n = N_START + step * N_STEP;

        for (in_idx = 0; in_idx < n_inputs; ++in_idx) {
            for (run = 0; run < RUNS; ++run) {
                unsigned long seed = base_seed + (unsigned long)(step * 1000 + in_idx * 100 + run);

                run_scenario("insertion", input_types[in_idx],
                             insertion_sort_int, n, run, seed);

                run_scenario("merge", input_types[in_idx],
                             merge_sort_int, n, run, seed);
            }
        }
    }

    return 0;
}
