/* bubble_time.c - bubble sort vs qsort on the same random data */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void bubble_sort(int a[], size_t n)
{
    size_t bound = n;
    while (bound > 1) {
        size_t last_swap = 0;
        for (size_t j = 1; j < bound; j++) {
            if (a[j] < a[j - 1]) {
                int tmp = a[j]; a[j] = a[j - 1]; a[j - 1] = tmp;
                last_swap = j;
            }
        }
        bound = last_swap;
    }
}

static int cmp_int(const void *pa, const void *pb)
{
    int a = *(const int *)pa, b = *(const int *)pb;
    return (a > b) - (a < b);       /* never a - b: that can overflow */
}

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

static int by_double(const void *pa, const void *pb)
{
    double a = *(const double *)pa, b = *(const double *)pb;
    return (a > b) - (a < b);
}

int main(void)
{
    const size_t sizes[] = { 1000, 5000, 20000 };
    enum { RUNS = 5 };
    unsigned int rng = 2463534242u;

    printf("%8s %14s %12s %8s\n", "n", "bubble (ms)", "qsort (ms)", "ratio");
    for (size_t s = 0; s < sizeof sizes / sizeof sizes[0]; s++) {
        size_t n = sizes[s];
        int *base = malloc(n * sizeof *base);
        int *x = malloc(n * sizeof *x);
        int *y = malloc(n * sizeof *y);
        if (!base || !x || !y) { fputs("out of memory\n", stderr); return 1; }

        for (size_t i = 0; i < n; i++) {
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            base[i] = (int)(rng % 1000000);
        }

        double tb[RUNS], tq[RUNS];
        for (int r = 0; r < RUNS; r++) {
            memcpy(x, base, n * sizeof *x);
            memcpy(y, base, n * sizeof *y);
            double t0 = now_ms(); bubble_sort(x, n);               tb[r] = now_ms() - t0;
            t0 = now_ms();        qsort(y, n, sizeof *y, cmp_int); tq[r] = now_ms() - t0;
            if (memcmp(x, y, n * sizeof *x) != 0) { puts("results differ"); return 1; }
        }
        qsort(tb, RUNS, sizeof tb[0], by_double);   /* median of five runs */
        qsort(tq, RUNS, sizeof tq[0], by_double);
        printf("%8zu %14.2f %12.2f %7.0fx\n", n, tb[RUNS / 2], tq[RUNS / 2],
               tb[RUNS / 2] / tq[RUNS / 2]);

        free(base); free(x); free(y);
    }
    return 0;
}
