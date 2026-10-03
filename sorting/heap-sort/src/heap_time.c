/* heap_time.c - heap sort, Floyd's variant and quicksort (Hoare, middle
 * pivot) on random ints from 10^4 to 10^7 elements; median of 5.
 * POSIX (clock_gettime). */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "heap_sort.h"

#define RUNS 5
#define MAXN 10000000

static void quick_rec(int a[], long lo, long hi)       /* for comparison */
{
    while (lo < hi) {
        int pivot = a[lo + (hi - lo) / 2];
        long i = lo - 1, j = hi + 1;
        for (;;) {
            do i++; while (a[i] < pivot);
            do j--; while (pivot < a[j]);
            if (i >= j) break;
            int t = a[i]; a[i] = a[j]; a[j] = t;
        }
        if (j - lo < hi - j) { quick_rec(a, lo, j); lo = j + 1; }
        else                 { quick_rec(a, j + 1, hi); hi = j; }
    }
}
static void quick_sort(int a[], size_t n) { if (n > 1) quick_rec(a, 0, (long)n - 1); }

static int cmp_double(const void *pa, const void *pb)
{
    double a = *(const double *)pa, b = *(const double *)pb;
    return (a > b) - (a < b);
}

static double time_ms(void (*fn)(int *, size_t), const int *in, int *w, size_t n)
{
    double t[RUNS];
    for (int r = 0; r < RUNS; r++) {
        memcpy(w, in, n * sizeof *w);
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        fn(w, n);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        t[r] = (double)(t1.tv_sec - t0.tv_sec) * 1e3 + (double)(t1.tv_nsec - t0.tv_nsec) / 1e6;
        for (size_t i = 1; i < n; i++)
            if (w[i] < w[i - 1]) { fputs("not sorted\n", stderr); exit(1); }
    }
    qsort(t, RUNS, sizeof t[0], cmp_double);
    return t[RUNS / 2];
}

int main(void)
{
    int *in = malloc(MAXN * sizeof *in), *w = malloc(MAXN * sizeof *w);
    if (!in || !w) { fputs("out of memory\n", stderr); return 1; }
    unsigned long long xs = 88172645463325252ULL;
    for (size_t i = 0; i < MAXN; i++) {
        xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17;
        in[i] = (int)(xs % 1000000000);
    }
    printf("ms, random ints, median of %d\n", RUNS);
    printf("%10s %11s %11s %11s %13s\n", "n", "heap sort", "floyd", "quicksort", "heap / quick");
    for (size_t n = 10000; n <= MAXN; n *= 10) {
        double h = time_ms(heap_sort, in, w, n), f = time_ms(heap_sort_floyd, in, w, n);
        double q = time_ms(quick_sort, in, w, n);
        printf("%10zu %11.2f %11.2f %11.2f %12.2fx\n", n, h, f, q, h / q);
    }
    free(in); free(w);
    return 0;
}
