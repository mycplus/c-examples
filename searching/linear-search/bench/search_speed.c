/* search_speed.c - nanoseconds per successful search in a sorted array of
 * distinct ints: linear search, sentinel search and binary search, for
 * sizes from 4 to 4,096. Each figure is the median of 5 runs of 2,000,000
 * searches for keys drawn at random from the array. Not run by ctest. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "linear_search.h"

#define SEARCHES 2000000
#define RUNS 5

static uint64_t state = 7;
static uint64_t next_random(void)    /* splitmix64 */
{
    uint64_t z = (state += 0x9e3779b97f4a7c15u);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9u;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebu;
    return z ^ (z >> 31);
}

static double now_ns(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static int cmp(const void *x, const void *y)
{
    double a = *(const double *)x, b = *(const double *)y;
    return (a > b) - (a < b);
}

int main(void)
{
    static int keys[SEARCHES];
    const size_t sizes[] = {4, 8, 16, 32, 64, 128, 256, 512, 1024, 4096};
    printf("%6s %10s %10s %10s\n", "n", "linear", "sentinel", "binary");
    for (size_t s = 0; s < sizeof sizes / sizeof sizes[0]; s++) {
        size_t n = sizes[s];
        int *a = malloc(n * sizeof *a);
        if (a == NULL)
            return 1;
        for (size_t i = 0; i < n; i++)
            a[i] = (int)(3 * i + 1);                 /* sorted, distinct */
        for (int k = 0; k < SEARCHES; k++)
            keys[k] = a[next_random() % n];
        double ns[3];
        long long check[3] = {0, 0, 0};    /* sum of the indices found */
        for (int method = 0; method < 3; method++) {
            double t[RUNS];
            for (int r = 0; r < RUNS; r++) {
                double start = now_ns();
                for (int k = 0; k < SEARCHES; k++)
                    check[method] += method == 0 ? linear_search(a, n, keys[k])
                                   : method == 1 ? sentinel_search(a, n, keys[k])
                                                 : binary_search(a, n, keys[k]);
                t[r] = (now_ns() - start) / SEARCHES;
            }
            qsort(t, RUNS, sizeof t[0], cmp);
            ns[method] = t[RUNS / 2];
        }
        if (check[0] != check[1] || check[0] != check[2]) {
            fprintf(stderr, "the three searches disagree\n");
            return 1;
        }
        printf("%6zu %10.1f %10.1f %10.1f\n", n, ns[0], ns[1], ns[2]);
        free(a);
    }
    return 0;
}
