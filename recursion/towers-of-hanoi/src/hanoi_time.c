/* hanoi_time.c - times both solvers with a visitor that only counts moves.
 * POSIX (clock_gettime). Build with -O2. Reports the median of 5 runs.
 * Usage: hanoi_time [MIN_N MAX_N]     (default 20 30)
 */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "hanoi.h"

#define RUNS 5

/* volatile, so the compiler cannot replace the counting with a formula */
static volatile unsigned long long counted;

static void count_move(unsigned disk, char source, char target, void *ctx)
{
    (void)disk; (void)source; (void)target; (void)ctx;
    counted = counted + 1;
}

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static double median_seconds(unsigned n, int iterative)
{
    double t[RUNS];
    for (int r = 0; r < RUNS; ++r) {
        counted = 0;
        double start = now();
        if (iterative)
            hanoi_iterative(n, 'A', 'C', 'B', count_move, NULL);
        else
            hanoi_recursive(n, 'A', 'C', 'B', count_move, NULL);
        t[r] = now() - start;
        uint64_t expected;
        hanoi_move_count(n, &expected);
        if (counted != expected) {
            fprintf(stderr, "n=%u: counted %llu moves\n", n, counted);
            exit(EXIT_FAILURE);
        }
    }
    qsort(t, RUNS, sizeof t[0], cmp_double);
    return t[RUNS / 2];
}

int main(int argc, char **argv)
{
    unsigned lo = 20, hi = 30;
    if (argc == 3) {
        lo = (unsigned)strtoul(argv[1], NULL, 10);
        hi = (unsigned)strtoul(argv[2], NULL, 10);
    }
    if (lo > hi || hi > 40) {
        fputs("usage: hanoi_time [MIN_N MAX_N], MAX_N <= 40\n", stderr);
        return 2;
    }

    printf(" n          moves  recursive s  ns/move  iterative s  ns/move\n");
    for (unsigned n = lo; n <= hi; ++n) {
        uint64_t moves;
        hanoi_move_count(n, &moves);
        double r = median_seconds(n, 0);
        double i = median_seconds(n, 1);
        printf("%2u %14llu %12.3f %8.2f %12.3f %8.2f\n", n,
               (unsigned long long)moves, r, r * 1e9 / (double)moves,
               i, i * 1e9 / (double)moves);
    }
    return 0;
}
