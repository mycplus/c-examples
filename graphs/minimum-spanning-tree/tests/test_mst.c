/* test_mst.c - checks the pieces one at a time, then Kruskal's algorithm
 * against the brute force on small random graphs and against a separate
 * Prim's algorithm on larger ones. Exits with status 1 on any failure. */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mst.h"

static Edge make_edge(int u, int v, int weight)
{
    Edge e;
    e.u = u;
    e.v = v;
    e.weight = weight;
    return e;
}

static int failures = 0;

#define CHECK(cond, ...)                                              \
    do {                                                              \
        if (!(cond)) {                                                \
            printf("FAILED line %d: ", __LINE__);                     \
            printf(__VA_ARGS__);                                      \
            printf("\n");                                             \
            failures++;                                               \
        }                                                             \
    } while (0)

static uint64_t rng_state = 2026;
static uint64_t next_random(void)    /* splitmix64 */
{
    uint64_t z = (rng_state += 0x9e3779b97f4a7c15u);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9u;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebu;
    return z ^ (z >> 31);
}
static int below(int n) { return (int)(next_random() % (uint64_t)n); }

/* The tree must have n - 1 edges taken from the input, join every vertex
 * and add up to the reported cost. */
static int is_spanning_tree(int n, const Edge *tree, const Edge *input, size_t m, long long cost)
{
    DisjointSet ds;
    if (!ds_init(&ds, n))
        return 0;
    long long total = 0;
    int ok = 1;
    for (int i = 0; i < n - 1 && ok; i++) {
        int found = 0;
        for (size_t k = 0; k < m && !found; k++)
            found = memcmp(&tree[i], &input[k], sizeof tree[i]) == 0;
        ok = found && ds_union(&ds, tree[i].u, tree[i].v);
        total += tree[i].weight;
    }
    ds_free(&ds);
    return ok && total == cost;
}

static void pieces(void)
{
    DisjointSet ds;
    CHECK(ds_init(&ds, 5), "ds_init");
    CHECK(ds_union(&ds, 0, 1) && ds_union(&ds, 3, 4) && ds_union(&ds, 1, 4), "three joins");
    CHECK(!ds_union(&ds, 0, 3), "0 and 3 are already joined");
    CHECK(ds_find(&ds, 0) == ds_find(&ds, 4) && ds_find(&ds, 2) == 2, "find");
    ds_free(&ds);

    Edge e[] = {{0, 1, INT_MAX}, {1, 2, INT_MIN}, {0, 2, 0}, {2, 3, 0}, {0, 3, -1}};
    sort_edges(e, 5);
    CHECK(e[0].weight == INT_MIN && e[1].weight == -1 && e[2].weight == 0 && e[4].weight == INT_MAX,
          "sort with extreme weights");
    CHECK(e[2].u == 0 && e[3].u == 2, "equal weights keep a fixed order");

    Edge tree[3];
    Edge apart[] = {{0, 1, 1}, {2, 3, 1}};
    long long cost;
    unsigned long long sets;
    CHECK(!kruskal(4, apart, 2, tree, &cost), "two separate parts have no spanning tree");
    CHECK(kruskal(1, NULL, 0, tree, &cost) && cost == 0 && kruskal(0, NULL, 0, tree, &cost),
          "0 and 1 vertices");
    CHECK(!mst_brute_force(4, apart, 2, &cost, &sets), "brute force: not connected");
    Edge negative[] = {{0, 1, -5}, {1, 2, -7}, {0, 2, 3}};
    CHECK(kruskal(3, negative, 3, tree, &cost) && cost == -12, "negative weights");
}

static void small_random_graphs(int trials)
{
    int agree = 0;
    for (int t = 0; t < trials; t++) {
        int n = 1 + below(8);
        size_t m = (size_t)below(13);
        Edge e[12], copy[12], tree[8];
        for (size_t k = 0; k < m; k++) {
            int u = below(n), v = below(n), w = below(21) - 10;   /* loops, repeats, negatives */
            e[k] = make_edge(u, v, w);
        }
        memcpy(copy, e, sizeof e);
        unsigned long long sets;
        long long want, got;
        bool exists = mst_brute_force(n, e, m, &want, &sets);
        bool found = kruskal(n, copy, m, tree, &got);
        int ok = found == exists && (!found || (got == want && is_spanning_tree(n, tree, e, m, got)));
        CHECK(ok, "n=%d m=%zu: kruskal %s %lld, brute force %s %lld", n, m,
              found ? "tree" : "none", got, exists ? "tree" : "none", want);
        agree += ok;
    }
    printf("small random graphs where Kruskal matches the brute force: %d of %d\n", agree, trials);
}

/* Prim's algorithm on an adjacency matrix: a different method, O(n^2). */
static long long prim(int n, const Edge *e, size_t m, bool *reachable)
{
    long long *w = malloc((size_t)n * (size_t)n * sizeof *w), *best = malloc((size_t)n * sizeof *best);
    char *in = calloc((size_t)n, 1);
    long long total = 0;
    *reachable = false;
    if (w == NULL || best == NULL || in == NULL)
        goto done;
    for (size_t k = 0; k < (size_t)n * (size_t)n; k++)
        w[k] = LLONG_MAX;
    for (size_t k = 0; k < m; k++) {
        size_t a = (size_t)e[k].u * (size_t)n + (size_t)e[k].v, b = (size_t)e[k].v * (size_t)n + (size_t)e[k].u;
        if (e[k].weight < w[a])
            w[a] = w[b] = e[k].weight;
    }
    for (int i = 0; i < n; i++)
        best[i] = LLONG_MAX;
    best[0] = 0;
    for (int round = 0; round < n; round++) {
        int pick = -1;
        for (int i = 0; i < n; i++)
            if (!in[i] && best[i] != LLONG_MAX && (pick < 0 || best[i] < best[pick]))
                pick = i;
        if (pick < 0)
            goto done;                       /* some vertex cannot be reached */
        in[pick] = 1;
        total += best[pick];
        for (int i = 0; i < n; i++) {
            long long c = w[(size_t)pick * (size_t)n + (size_t)i];
            if (!in[i] && c < best[i])
                best[i] = c;
        }
    }
    *reachable = true;
done:
    free(w);
    free(best);
    free(in);
    return total;
}

static void larger_random_graphs(int trials)
{
    int agree = 0;
    for (int t = 0; t < trials; t++) {
        int n = 2 + below(299);
        size_t m = (size_t)n + (size_t)below(4 * n);
        Edge *e = malloc(m * sizeof *e), *tree = malloc((size_t)n * sizeof *tree);
        if (e == NULL || tree == NULL) {
            CHECK(0, "out of memory");
            free(e);
            free(tree);
            return;
        }
        for (size_t k = 0; k < m; k++) {
            int u = below(n), v = below(n), w = below(2000001) - 1000000;
            e[k] = make_edge(u, v, w);
        }
        bool reachable;
        long long want = prim(n, e, m, &reachable), got = 0;
        bool found = kruskal(n, e, m, tree, &got);
        int ok = found == reachable && (!found || got == want);
        CHECK(ok, "n=%d m=%zu: kruskal %lld, prim %lld", n, m, got, want);
        agree += ok;
        free(e);
        free(tree);
    }
    printf("larger random graphs where Kruskal matches Prim's algorithm: %d of %d\n", agree, trials);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    pieces();
    small_random_graphs(5000);
    larger_random_graphs(300);
    printf("%d failed\n", failures);
    return failures != 0;
}
