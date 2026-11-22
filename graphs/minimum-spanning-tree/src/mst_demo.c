/* mst_demo.c - the worked example from the article: connect six offices with
 * the cheapest set of network cables. Prints Kruskal's decisions one edge at
 * a time, checks the answer with the brute force, and counts how much work
 * each approach does as the network grows. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "mst.h"

static Edge make_edge(int u, int v, int weight)
{
    Edge e;
    e.u = u;
    e.v = v;
    e.weight = weight;
    return e;
}

static const char NAMES[] = "ABCDEF";

static void print_edges(const Edge *e, size_t m)
{
    for (size_t i = 0; i < m; i++)
        printf("  %c-%c %2d\n", NAMES[e[i].u], NAMES[e[i].v], e[i].weight);
}

/* Kruskal's loop again, written out so it can explain each decision. */
static void trace(int n, Edge *edges, size_t m)
{
    DisjointSet ds;
    if (!ds_init(&ds, n))
        return;
    sort_edges(edges, m);
    int taken = 0;
    long long total = 0;
    for (size_t i = 0; i < m && taken < n - 1; i++) {
        const Edge *e = &edges[i];
        printf("  %c-%c %2d  ", NAMES[e->u], NAMES[e->v], e->weight);
        if (ds_union(&ds, e->u, e->v)) {
            taken++;
            total += e->weight;
            printf("take   (cables so far: %d, cost %lld)\n", taken, total);
        } else {
            printf("skip   (%c and %c are already connected)\n", NAMES[e->u], NAMES[e->v]);
        }
    }
    ds_free(&ds);
}

/* A connected graph on n vertices with m edges: a random path through all
 * vertices, then random extra edges. The same graph on every platform. */
static void make_graph(int n, size_t m, Edge *e, uint32_t seed)
{
    uint32_t s = seed;
    for (int i = 1; i < n; i++) {
        s = s * 1664525u + 1013904223u;
        e[i - 1] = make_edge((int)((s >> 8) % (uint32_t)i), i, 1 + (int)((s >> 16) % 50));
    }
    for (size_t k = (size_t)n - 1; k < m; k++) {
        int u, v;
        do {
            s = s * 1664525u + 1013904223u;
            u = (int)((s >> 8) % (uint32_t)n);
            v = (int)((s >> 20) % (uint32_t)n);
        } while (u == v);
        e[k] = make_edge(u, v, 1 + (int)((s >> 4) % 50));
    }
}

int main(void)
{
    Edge offices[] = {
        {0, 1, 4}, {0, 2, 3}, {1, 2, 1}, {1, 3, 2}, {2, 3, 4},
        {3, 4, 2}, {4, 5, 6}, {3, 5, 7}, {2, 4, 5}, {1, 5, 9},
    };
    const size_t m = sizeof offices / sizeof offices[0];
    Edge tree[5];

    printf("Possible cables (offices A to F, cost):\n");
    print_edges(offices, m);

    unsigned long long sets;
    long long brute, cost;
    mst_brute_force(6, offices, m, &brute, &sets);
    printf("\nBrute force: cheapest network costs %lld, after checking %llu sets of 5 cables\n",
           brute, sets);

    printf("\nKruskal, cheapest cable first:\n");
    trace(6, offices, m);
    kruskal(6, offices, m, tree, &cost);
    printf("\nKruskal's network (cost %lld):\n", cost);
    print_edges(tree, 5);
    printf("Same cost as the brute force: %s\n", cost == brute ? "yes" : "NO");

    printf("\nWork as the network grows (n offices, 2n possible cables):\n");
    printf("%3s %6s %22s %15s\n", "n", "cables", "brute-force sets", "Kruskal sorts");
    for (int n = 4; n <= 12; n += 2) {
        size_t mm = 2 * (size_t)n;
        Edge e[24], t[11];
        make_graph(n, mm, e, (uint32_t)n);
        long long b, k;
        bool same = mst_brute_force(n, e, mm, &b, &sets) && kruskal(n, e, mm, t, &k) && b == k;
        printf("%3d %6zu %22llu %8zu cables%s\n", n, mm, sets, mm, same ? "" : "  MISMATCH");
    }
    return 0;
}
