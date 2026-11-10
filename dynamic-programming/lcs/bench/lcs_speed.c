/* lcs_speed.c - time and memory of the two LCS functions on random DNA
 * strings of equal length: lcs_length keeps two rows, lcs_string keeps the
 * whole table so it can walk back. Median of 5 runs. Then a finer sweep of
 * lcs_length alone, to show where the two rows outgrow the L1 data cache.
 * POSIX clock; built, not run, by the tests. */
#define _POSIX_C_SOURCE 199309L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "lcs.h"

#define RUNS 5

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static int cmp(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static uint64_t rng_state = 99;
static char random_base(void)
{
    rng_state = rng_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return "ACGT"[rng_state >> 62];
}

int main(void)
{
    puts("    n   LCS   lcs_length ms  rows MB   lcs_string ms  table MB   ns per cell");
    static const size_t sizes[] = {1000, 2000, 4000, 8000};
    for (size_t s = 0; s < sizeof sizes / sizeof sizes[0]; ++s) {
        size_t n = sizes[s];
        char *a = malloc(n + 1), *b = malloc(n + 1);
        if (a == NULL || b == NULL)
            return 1;
        for (size_t i = 0; i < n; ++i) {
            a[i] = random_base();
            b[i] = random_base();
        }
        a[n] = b[n] = '\0';
        double tl[RUNS], ts[RUNS];
        size_t len = 0, slen = 0;
        for (int r = 0; r < RUNS; ++r) {
            double t0 = now();
            len = lcs_length(a, b);
            tl[r] = now() - t0;
            t0 = now();
            char *str = lcs_string(a, b);
            ts[r] = now() - t0;
            if (str == NULL)
                return 1;
            slen = strlen(str);
            free(str);
        }
        if (len != slen)
            return 1;
        qsort(tl, RUNS, sizeof *tl, cmp);
        qsort(ts, RUNS, sizeof *ts, cmp);
        double cells = (double)(n + 1) * (double)(n + 1);
        printf("%5zu %5zu %15.1f %8.2f %15.1f %9.1f %13.2f\n", n, len, tl[RUNS / 2] * 1e3,
               2.0 * (double)(n + 1) * sizeof(size_t) / 1e6, ts[RUNS / 2] * 1e3,
               cells * sizeof(uint32_t) / 1e6, tl[RUNS / 2] / cells * 1e9);
        free(a);
        free(b);
    }

    puts("\nlcs_length only, best of 3:\n    n  two rows KB  ns per cell");
    for (size_t n = 1000; n <= 12000; n += 1000) {
        char *a = malloc(n + 1), *b = malloc(n + 1);
        if (a == NULL || b == NULL)
            return 1;
        for (size_t i = 0; i < n; ++i) {
            a[i] = random_base();
            b[i] = random_base();
        }
        a[n] = b[n] = '\0';
        double best = 1e9;
        for (int r = 0; r < 3; ++r) {
            double t0 = now();
            volatile size_t len = lcs_length(a, b);
            (void)len;
            double t = now() - t0;
            if (t < best)
                best = t;
        }
        double cells = (double)(n + 1) * (double)(n + 1);
        printf("%5zu %12.0f %12.2f\n", n, 2.0 * (double)(n + 1) * sizeof(size_t) / 1024.0,
               best / cells * 1e9);
        free(a);
        free(b);
    }
    return 0;
}
