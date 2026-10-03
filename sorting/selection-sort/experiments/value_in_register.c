/* value_in_register.c - selection sort that keeps the current minimum
 * value in a local variable instead of re-reading a[min] on every
 * comparison. Median of 5 runs on 20,000 random and 20,000 sorted ints.
 * Compare with selection_time's "selection sort" row. POSIX. */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N 20000
#define RUNS 5

void selection_sort_value(int a[], size_t n)
{
    for (size_t i = 0; i + 1 < n; i++) {
        size_t min = i;
        int minv = a[i];                 /* the minimum so far, in a local */
        for (size_t j = i + 1; j < n; j++)
            if (a[j] < minv) {
                minv = a[j];
                min = j;
            }
        if (min != i) {
            a[min] = a[i];
            a[i] = minv;
        }
    }
}

static double median_ms(const int *in, int *work)
{
    double t[RUNS];
    for (int r = 0; r < RUNS; r++) {
        memcpy(work, in, N * sizeof *work);
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        selection_sort_value(work, N);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        t[r] = (double)(t1.tv_sec - t0.tv_sec) * 1e3 + (double)(t1.tv_nsec - t0.tv_nsec) / 1e6;
        for (size_t i = 1; i < N; i++)
            if (work[i] < work[i - 1]) { fputs("not sorted\n", stderr); exit(1); }
    }
    for (int x = 1; x < RUNS; x++)                  /* insertion sort, 5 values */
        for (int y = x; y > 0 && t[y] < t[y - 1]; y--) {
            double tmp = t[y]; t[y] = t[y - 1]; t[y - 1] = tmp;
        }
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
    printf("value in a local, ms: random %.1f  sorted %.1f\n",
           median_ms(random_in, work), median_ms(sorted_in, work));
    return 0;
}
