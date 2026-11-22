/* no_cycle_check.c - "greedy" without its safety check: take the n - 1
 * cheapest cables and stop. The total is lower than the true minimum because
 * the result is not a spanning tree at all. */
#include <stdio.h>
#include <stdlib.h>

typedef struct { int u, v, weight; } Edge;

/* Cheapest first; equal costs in vertex order, because qsort is not stable
 * and the C libraries disagree on how to order ties. */
static int by_weight(const void *x, const void *y)
{
    const Edge *a = x, *b = y;
    if (a->weight != b->weight)
        return (a->weight > b->weight) - (a->weight < b->weight);
    if (a->u != b->u)
        return (a->u > b->u) - (a->u < b->u);
    return (a->v > b->v) - (a->v < b->v);
}

/* Which offices a set of cables reaches from office 0 (repeated scans). */
static int reached(const Edge *e, int count)
{
    int seen[8] = {1}, total = 1, changed = 1;
    while (changed) {
        changed = 0;
        for (int i = 0; i < count; i++)
            if (seen[e[i].u] != seen[e[i].v]) {
                seen[e[i].u] = seen[e[i].v] = 1;
                total++;
                changed = 1;
            }
    }
    return total;
}

int main(void)
{
    Edge cables[] = {
        {0, 1, 4}, {0, 2, 3}, {1, 2, 1}, {1, 3, 2}, {2, 3, 4},
        {3, 4, 2}, {4, 5, 6}, {3, 5, 7}, {2, 4, 5}, {1, 5, 9},
    };
    const int n = 6;
    qsort(cables, sizeof cables / sizeof cables[0], sizeof cables[0], by_weight);
    int cost = 0;
    printf("the %d cheapest cables:", n - 1);
    for (int i = 0; i < n - 1; i++) {
        printf(" %c-%c", "ABCDEF"[cables[i].u], "ABCDEF"[cables[i].v]);
        cost += cables[i].weight;
    }
    printf("\ncost %d, offices connected to A: %d of %d\n", cost, reached(cables, n - 1), n);
    return 0;
}
