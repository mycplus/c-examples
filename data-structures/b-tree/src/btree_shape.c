/* btree_shape.c - how tall and how full a B-tree gets: one million keys
 * inserted in random and in ascending order, for several minimum degrees.
 * Reports the height, the bound on height, how full the nodes are, and the
 * average number of nodes a search reads.
 * Usage: btree_shape [n]   (default 1000000) */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "btree.h"

static uint64_t rng_state = 42;
static uint64_t next_random(void)                   /* splitmix64 */
{
    uint64_t z = (rng_state += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

int main(int argc, char **argv)
{
    long n = argc > 1 ? strtol(argv[1], NULL, 10) : 1000000;
    if (n < 1 || n > 100000000)
        return 2;
    btree_key *order = malloc((size_t)n * sizeof *order);
    if (order == NULL)
        return 1;

    static const int degrees[] = {2, 4, 16, 64, 256};
    printf("%ld keys\n", n);
    printf("%-10s %4s %7s %7s %10s %7s %12s\n", "order", "t", "height", "bound", "nodes",
           "full", "reads/search");
    for (int pass = 0; pass < 2; ++pass) {
        for (long i = 0; i < n; ++i)
            order[i] = i;
        if (pass == 0)                                /* Fisher-Yates shuffle */
            for (long i = n - 1; i > 0; --i) {
                long j = (long)(next_random() % (uint64_t)(i + 1));
                btree_key tmp = order[i];
                order[i] = order[j];
                order[j] = tmp;
            }
        for (size_t d = 0; d < sizeof degrees / sizeof degrees[0]; ++d) {
            int t = degrees[d];
            btree *tree = btree_create(t);
            if (tree == NULL)
                return 1;
            for (long i = 0; i < n; ++i)
                if (btree_insert(tree, order[i], 0) < 0)
                    return 1;
            if (btree_check(tree) != NULL)
                return 1;
            btree_stats s = btree_get_stats(tree);
            /* At most 1 + log_t((n+1)/2) levels (Cormen et al., Theorem 18.1). */
            int bound = 1 + (int)floor(log((double)(n + 1) / 2.0) / log((double)t));
            double full = (double)s.keys / ((double)s.nodes * (2.0 * t - 1.0));
            long reads = 0;
            for (long k = 0; k < n; ++k)
                reads += btree_search_cost(tree, k);
            printf("%-10s %4d %7d %7d %10zu %6.1f%% %12.2f\n", pass == 0 ? "random" : "ascending",
                   t, s.height, bound, s.nodes, 100.0 * full, (double)reads / (double)n);
            btree_destroy(tree);
        }
    }
    free(order);
    return 0;
}
