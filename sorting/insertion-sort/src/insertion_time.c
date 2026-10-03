/* insertion_time.c - (1) n = 20,000 random ints; (2) many small arrays,
 * where insertion sort competes with qsort(). Median of 5 runs each.
 * POSIX (clock_gettime). */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "insertion_sort.h"

#define RUNS 5
#define TOTAL 2000000                    /* elements per small-array run */

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
static void qsort_int(int a[], size_t n) { qsort(a, n, sizeof a[0], cmp_int); }

static double now_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec * 1e3 + (double)t.tv_nsec / 1e6;
}

/* sorts the TOTAL values in `in` as consecutive arrays of n; median ms */
static double time_chunks(void (*fn)(int *, size_t), const int *in, int *work,
                          size_t total, size_t n)
{
    double t[RUNS];
    for (int r = 0; r < RUNS; r++) {
        memcpy(work, in, total * sizeof *work);
        double t0 = now_ms();
        for (size_t off = 0; off + n <= total; off += n)
            fn(work + off, n);
        t[r] = now_ms() - t0;
        for (size_t off = 0; off + n <= total; off += n)     /* check */
            for (size_t i = off + 1; i < off + n; i++)
                if (work[i] < work[i - 1]) { fputs("not sorted\n", stderr); exit(1); }
    }
    qsort(t, RUNS, sizeof t[0], cmp_double);
    return t[RUNS / 2];
}

int main(void)
{
    int *in = malloc(TOTAL * sizeof *in), *work = malloc(TOTAL * sizeof *work);
    if (!in || !work) { fputs("out of memory\n", stderr); return 1; }
    unsigned long long xs = 88172645463325252ULL;
    for (size_t i = 0; i < TOTAL; i++) {
        xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17;
        in[i] = (int)(xs % 1000000000);
    }

    printf("one array of 20000 random ints, ms (median of %d)\n", RUNS);
    printf("  insertion sort         %9.2f\n", time_chunks(insertion_sort, in, work, 20000, 20000));
    printf("  binary insertion sort  %9.2f\n", time_chunks(binary_insertion_sort, in, work, 20000, 20000));
    printf("  qsort()                %9.2f\n", time_chunks(qsort_int, in, work, 20000, 20000));

    printf("\n%d ints sorted as arrays of n, ns per element\n", TOTAL);
    printf("%8s %12s %12s %12s\n", "n", "insertion", "binary ins.", "qsort()");
    static const size_t sizes[] = { 4, 8, 16, 32, 64, 128, 256, 512 };
    for (size_t k = 0; k < sizeof sizes / sizeof sizes[0]; k++) {
        size_t n = sizes[k];
        double f = 1e6 / TOTAL;                     /* ms per run -> ns per element */
        printf("%8zu %12.1f %12.1f %12.1f\n", n,
               time_chunks(insertion_sort, in, work, TOTAL, n) * f,
               time_chunks(binary_insertion_sort, in, work, TOTAL, n) * f,
               time_chunks(qsort_int, in, work, TOTAL, n) * f);
    }
    free(in); free(work);
    return 0;
}
