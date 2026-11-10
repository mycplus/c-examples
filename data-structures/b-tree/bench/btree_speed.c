/* btree_speed.c - in-memory speed of the B-tree for several minimum
 * degrees: insert, search and remove of one million random keys, median of
 * 5 runs. A binary search over a sorted array of the same keys is timed as
 * a baseline for search. POSIX clock; built, not run, by the tests.
 * Usage: btree_speed [n] */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "btree.h"

#define RUNS 5

static uint64_t rng_state = 7;
static uint64_t next_random(void)                   /* splitmix64 */
{
    uint64_t z = (rng_state += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
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

static int cmp_key(const void *a, const void *b)
{
    btree_key x = *(const btree_key *)a, y = *(const btree_key *)b;
    return (x > y) - (x < y);
}

static double median(double *t)
{
    qsort(t, RUNS, sizeof *t, cmp_double);
    return t[RUNS / 2];
}

int main(int argc, char **argv)
{
    long n = argc > 1 ? strtol(argv[1], NULL, 10) : 1000000;
    btree_key *keys = malloc((size_t)n * sizeof *keys), *sorted = malloc((size_t)n * sizeof *sorted);
    if (n < 1 || keys == NULL || sorted == NULL)
        return 1;
    for (long i = 0; i < n; ++i)
        keys[i] = sorted[i] = (btree_key)(next_random() >> 1);   /* distinct with overwhelming probability */
    qsort(sorted, (size_t)n, sizeof *sorted, cmp_key);

    printf("%ld random keys, median of %d runs, ns per operation\n", n, RUNS);
    printf("%-22s %8s %8s %8s\n", "structure", "insert", "search", "remove");
    static const int degrees[] = {2, 4, 8, 16, 32, 64, 128, 256};
    volatile long found = 0;
    for (size_t d = 0; d < sizeof degrees / sizeof degrees[0]; ++d) {
        double ti[RUNS], ts[RUNS], tr[RUNS];
        for (int r = 0; r < RUNS; ++r) {
            btree *tree = btree_create(degrees[d]);
            double s = now();
            for (long i = 0; i < n; ++i)
                btree_insert(tree, keys[i], i);
            ti[r] = now() - s;
            s = now();
            for (long i = n - 1; i >= 0; --i)
                found += btree_search(tree, keys[i], NULL);
            ts[r] = now() - s;
            s = now();
            for (long i = 0; i < n; ++i)
                btree_remove(tree, keys[i]);
            tr[r] = now() - s;
            btree_destroy(tree);
        }
        char label[32];
        snprintf(label, sizeof label, "B-tree, t = %d", degrees[d]);
        printf("%-22s %8.0f %8.0f %8.0f\n", label, median(ti) / (double)n * 1e9,
               median(ts) / (double)n * 1e9, median(tr) / (double)n * 1e9);
    }
    double tb[RUNS];
    for (int r = 0; r < RUNS; ++r) {
        double s = now();
        for (long i = n - 1; i >= 0; --i)
            found += bsearch(&keys[i], sorted, (size_t)n, sizeof *sorted, cmp_key) != NULL;
        tb[r] = now() - s;
    }
    printf("%-22s %8s %8.0f %8s\n", "sorted array, bsearch", "-", median(tb) / (double)n * 1e9, "-");
    printf("searches that found their key: %ld of %ld\n", found / (8 * RUNS + RUNS), n);
    free(keys);
    free(sorted);
    return 0;
}
