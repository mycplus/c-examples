/* merge_time.c - median of 5 timed runs on 1,000,000 ints, random and
 * sorted. POSIX (clock_gettime). */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merge_sort.h"

#define N 1000000
#define RUNS 5

static int cmp_int(const void *pa, const void *pb)
{
    int a = *(const int *)pa, b = *(const int *)pb;
    return (a > b) - (a < b);
}
static int cmp_double(const void *pa, const void *pb)
{
    double a = *(const double *)pa, b = *(const double *)pb;
    return (a > b) - (a < b);
}
static int top_down(int *a, size_t n)  { return merge_sort(a, n); }
static int bottom_up(int *a, size_t n) { return merge_sort_bottom_up(a, n); }
static int glibc_qsort(int *a, size_t n) { qsort(a, n, sizeof *a, cmp_int); return 0; }

static double time_ms(int (*fn)(int *, size_t), const int *in, int *work)
{
    double t[RUNS];
    for (int r = 0; r < RUNS; r++) {
        memcpy(work, in, N * sizeof *work);
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        if (fn(work, N) != 0) { fputs("out of memory\n", stderr); exit(1); }
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
    int *random_in = malloc(N * sizeof *random_in), *sorted_in = malloc(N * sizeof *sorted_in);
    int *work = malloc(N * sizeof *work);
    if (!random_in || !sorted_in || !work) { fputs("out of memory\n", stderr); return 1; }
    unsigned long long xs = 88172645463325252ULL;
    for (size_t i = 0; i < N; i++) {
        xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17;
        random_in[i] = (int)(xs % 1000000000);
        sorted_in[i] = (int)i;
    }
    static const struct { const char *name; int (*fn)(int *, size_t); } v[] = {
        { "top-down merge sort", top_down },
        { "bottom-up merge sort", bottom_up },
        { "qsort() glibc", glibc_qsort },
    };
    printf("ms, n = %d, median of %d     random     sorted\n", N, RUNS);
    for (size_t k = 0; k < sizeof v / sizeof v[0]; k++)
        printf("%-32s %9.1f  %9.1f\n", v[k].name,
               time_ms(v[k].fn, random_in, work), time_ms(v[k].fn, sorted_in, work));
    free(random_in); free(sorted_in); free(work);
    return 0;
}
