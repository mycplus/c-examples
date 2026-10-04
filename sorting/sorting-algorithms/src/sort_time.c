/* sort_time.c - median of 5 timed runs per algorithm on random int input.
 * Build: gcc -std=c11 -Wall -Wextra -O2 sort_time.c sorting.c -o sort_time */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sorting.h"

#define RUNS 5

static int cmp_int(const void *pa, const void *pb)
{
    int a = *(const int *)pa, b = *(const int *)pb;
    return (a > b) - (a < b);
}

static void merge_sort_v(int a[], size_t n)
{
    if (merge_sort(a, n) != 0) { fputs("out of memory\n", stderr); exit(1); }
}
static void qsort_v(int a[], size_t n) { qsort(a, n, sizeof a[0], cmp_int); }

static int cmp_double(const void *pa, const void *pb)
{
    double a = *(const double *)pa, b = *(const double *)pb;
    return (a > b) - (a < b);
}

static double ms_since(struct timespec t0)
{
    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    return (double)(t1.tv_sec - t0.tv_sec) * 1e3 + (double)(t1.tv_nsec - t0.tv_nsec) / 1e6;
}

static double time_sort(void (*fn)(int *, size_t), const int *input, const int *ref,
                        int *work, size_t n)
{
    double t[RUNS];
    for (int r = 0; r < RUNS; r++) {
        memcpy(work, input, n * sizeof *work);
        struct timespec t0;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        fn(work, n);
        t[r] = ms_since(t0);
        if (memcmp(work, ref, n * sizeof *work) != 0) {
            fputs("wrong result\n", stderr);
            exit(1);
        }
    }
    qsort(t, RUNS, sizeof t[0], cmp_double);
    return t[RUNS / 2];
}

int main(void)
{
    static const struct { const char *name; void (*fn)(int *, size_t); int quadratic; } algos[] = {
        { "insertion sort",     insertion_sort,  1 },
        { "selection sort",     selection_sort,  1 },
        { "bubble sort",        bubble_sort,     1 },
        { "shell sort (Knuth)", shell_sort,      0 },
        { "merge sort",         merge_sort_v,    0 },
        { "heap sort",          heap_sort,       0 },
        { "quicksort (middle)", quick_sort,      0 },
        { "quicksort (last)",   quick_sort_last, 0 },
        { "qsort() glibc",      qsort_v,         0 },
    };
    const size_t sizes[] = { 20000, 1000000 };
    unsigned long long xs = 88172645463325252ULL;

    int *input = malloc(1000000 * sizeof *input);
    int *ref   = malloc(1000000 * sizeof *ref);
    int *work  = malloc(1000000 * sizeof *work);
    if (!input || !ref || !work) { fputs("out of memory\n", stderr); return 1; }

    for (size_t i = 0; i < 1000000; i++) {
        xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17;
        input[i] = (int)(xs % 1000000000);
    }

    printf("%-20s %14s %14s\n", "algorithm (ms)", "n = 20000", "n = 1000000");
    for (size_t k = 0; k < sizeof algos / sizeof algos[0]; k++) {
        printf("%-20s", algos[k].name);
        for (size_t s = 0; s < 2; s++) {
            size_t n = sizes[s];
            if (algos[k].quadratic && n > 20000) { printf("%15s", "-"); continue; }
            memcpy(ref, input, n * sizeof *ref);
            qsort(ref, n, sizeof ref[0], cmp_int);
            printf("%15.2f", time_sort(algos[k].fn, input, ref, work, n));
        }
        printf("\n");
    }
    free(input); free(ref); free(work);
    return 0;
}
