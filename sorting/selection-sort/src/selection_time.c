/* selection_time.c - median of 5 timed runs, n = 20,000, random and sorted
 * input. POSIX (clock_gettime). */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "selection_sort.h"

#define N 20000
#define RUNS 5

/* The swap-as-you-go loop sometimes presented as selection sort. */
static void exchange_sort(int a[], size_t n)
{
    for (size_t i = 0; i + 1 < n; i++)
        for (size_t j = i + 1; j < n; j++)
            if (a[j] < a[i]) {
                int t = a[i]; a[i] = a[j]; a[j] = t;
            }
}

static int cmp_double(const void *pa, const void *pb)
{
    double a = *(const double *)pa, b = *(const double *)pb;
    return (a > b) - (a < b);
}

static double time_ms(void (*fn)(int *, size_t), const int *in, int *work)
{
    double t[RUNS];
    for (int r = 0; r < RUNS; r++) {
        memcpy(work, in, N * sizeof *work);
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        fn(work, N);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        t[r] = (double)(t1.tv_sec - t0.tv_sec) * 1e3 + (double)(t1.tv_nsec - t0.tv_nsec) / 1e6;
        for (size_t i = 1; i < N; i++)
            if (work[i] < work[i - 1]) { fputs("not sorted\n", stderr); exit(1); }
    }
    qsort(t, RUNS, sizeof t[0], cmp_double);
    return t[RUNS / 2];
}

int main(void)
{
    static int random_in[N], sorted_in[N], work[N];
    unsigned long long xs = 88172645463325252ULL;
    for (size_t i = 0; i < N; i++) {
        xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17;
        random_in[i] = (int)(xs % 1000000000);
        sorted_in[i] = (int)i;
    }
    static const struct { const char *name; void (*fn)(int *, size_t); } v[] = {
        { "selection sort", selection_sort },
        { "stable selection", stable_selection_sort },
        { "double selection", double_selection_sort },
        { "exchange sort", exchange_sort },
    };
    printf("ms, n = %d, median of %d   random     sorted\n", N, RUNS);
    for (size_t k = 0; k < sizeof v / sizeof v[0]; k++)
        printf("%-28s %9.1f  %9.1f\n", v[k].name,
               time_ms(v[k].fn, random_in, work), time_ms(v[k].fn, sorted_in, work));
    return 0;
}
