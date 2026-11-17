/* Tests for the graph loader, Dijkstra and Floyd-Warshall.

   Besides hand-made cases, both algorithms are checked against Bellman-Ford,
   a slower algorithm with a much simpler proof, on thousands of random
   graphs: with positive weights, with zero weights, with negative weights
   (Floyd-Warshall only) and with negative cycles. Every reconstructed path
   is also walked edge by edge to confirm it exists and adds up. */
#include "graph.h"
#include "shortest_path.h"

#include <stdlib.h>
#include <string.h>

static int checks, failures;
#define CHECK(cond)                                                                    \
    do {                                                                               \
        checks++;                                                                      \
        if (!(cond)) {                                                                 \
            failures++;                                                                \
            fprintf(stderr, "FAILED line %d: %s\n", __LINE__, #cond);                  \
        }                                                                              \
    } while (0)

static unsigned long long rng_state = 88172645463325252ULL;
static unsigned rnd(unsigned bound)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return (unsigned)(rng_state % bound);
}

/* ---- loading maps ---- */

static bool load_text(graph *g, const char *text, char *err, size_t len)
{
    FILE *f = tmpfile();
    if (f == NULL) {
        fprintf(stderr, "tmpfile failed\n");
        exit(1);
    }
    fputs(text, f);
    rewind(f);
    graph_init(g);
    bool ok = graph_load_map(g, f, err, len);
    fclose(f);
    return ok;
}

static void expect_error(const char *text, const char *needle)
{
    graph g;
    char err[128] = "";
    CHECK(!load_text(&g, text, err, sizeof err));
    CHECK(strstr(err, needle) != NULL);
    graph_free(&g);
}

static void test_loader(void)
{
    graph g;
    char err[128];
    CHECK(load_text(&g, "# comment\n\n  A town , B ,  7 \r\nB,C,3\n", err, sizeof err));
    CHECK(g.n == 3);
    CHECK(graph_find(&g, "A town") == 0 && graph_find(&g, "C") == 2 && graph_find(&g, "D") == -1);
    CHECK(g.deg[1] == 2);                               /* B has a road to A and to C */
    graph_free(&g);

    expect_error("A,B\n", "line 1:");
    expect_error("A,B,5\nA,B,C,5\n", "line 2:");
    expect_error("A,B,0\n", "from 1 to");
    expect_error("A,B,-4\n", "from 1 to");
    expect_error("A,B,5km\n", "from 1 to");
    expect_error("A,B,99999999999999999999\n", "from 1 to");
    expect_error(",B,5\n", "two town names");
    expect_error("A,A,5\n", "to itself");
    expect_error("# nothing here\n", "no roads");

    char longline[400];
    memset(longline, 'x', sizeof longline - 2);
    longline[sizeof longline - 2] = '\n';
    longline[sizeof longline - 1] = '\0';
    expect_error(longline, "longer than");
}

/* ---- the Lakeland map ---- */

static void test_lakeland(const char *map_path)
{
    FILE *f = fopen(map_path, "r");
    CHECK(f != NULL);
    if (f == NULL)
        return;
    graph g;
    graph_init(&g);
    char err[128];
    CHECK(graph_load_map(&g, f, err, sizeof err));
    fclose(f);

    int n = g.n, a = graph_find(&g, "Ashford"), fair = graph_find(&g, "Fairhaven"),
        lark = graph_find(&g, "Larkspur"), seal = graph_find(&g, "Seal Rock");
    dist_t *dist = malloc((size_t)n * sizeof *dist), *all = malloc((size_t)n * (size_t)n * sizeof *all);
    int *prev = malloc((size_t)n * sizeof *prev), *next = malloc((size_t)n * (size_t)n * sizeof *next);
    int *path = malloc((size_t)n * sizeof *path);

    CHECK(dijkstra(&g, a, dist, prev));
    CHECK(dist[lark] == 360);
    CHECK(dist[fair] == 216);                           /* the 240 km direct road is not the shortest */
    CHECK(dist[seal] == DIST_INF && prev[seal] == -1);  /* the island */
    CHECK(path_from_prev(prev, a, seal, path, n) == 0);
    CHECK(path_from_prev(prev, a, lark, path, n) == 7);
    CHECK(path_from_prev(prev, a, lark, path, 3) == 0); /* too small a buffer */
    CHECK(path_from_prev(prev, a, a, path, n) == 1 && path[0] == a);

    CHECK(floyd_warshall(&g, all, next));
    for (int v = 0; v < n; v++)
        CHECK(all[a * n + v] == dist[v]);
    CHECK(path_from_next(next, n, a, seal, path, n) == 0);
    CHECK(path_from_next(next, n, lark, a, path, n) == 7 && path[0] == lark && path[6] == a);

    free(dist);
    free(all);
    free(prev);
    free(next);
    free(path);
    graph_free(&g);
}

/* ---- small hand-made graphs ---- */

static void make_nodes(graph *g, int n)
{
    char name[16];
    graph_init(g);
    for (int i = 0; i < n; i++) {
        snprintf(name, sizeof name, "%d", i);
        graph_add_node(g, name);
    }
}

static void test_special_edges(void)
{
    graph g;
    dist_t dist[4], all[16];
    int prev[4], next[16], path[4];

    make_nodes(&g, 3);                                  /* parallel roads and a zero-length one */
    graph_add_edge(&g, 0, 1, 9);
    graph_add_edge(&g, 0, 1, 4);
    graph_add_edge(&g, 0, 1, 7);
    graph_add_edge(&g, 1, 2, 0);
    CHECK(dijkstra(&g, 0, dist, prev));
    CHECK(dist[1] == 4 && dist[2] == 4);
    CHECK(floyd_warshall(&g, all, next));
    CHECK(all[0 * 3 + 1] == 4 && all[0 * 3 + 2] == 4);
    CHECK(all[2 * 3 + 0] == DIST_INF);                  /* these roads are one-way */
    graph_free(&g);

    make_nodes(&g, 4);                                  /* the electric van: a downhill road */
    graph_add_edge(&g, 0, 1, 800);
    graph_add_edge(&g, 0, 2, 500);
    graph_add_edge(&g, 1, 2, -400);
    graph_add_edge(&g, 2, 3, 300);
    CHECK(!dijkstra(&g, 0, dist, prev));                /* refuses a negative weight */
    CHECK(floyd_warshall(&g, all, next));
    CHECK(all[0 * 4 + 3] == 700);
    CHECK(path_from_next(next, 4, 0, 3, path, 4) == 4 && path[1] == 1 && path[2] == 2);
    graph_free(&g);

    make_nodes(&g, 3);                                  /* a negative cycle 0 -> 1 -> 2 -> 0 */
    graph_add_edge(&g, 0, 1, 2);
    graph_add_edge(&g, 1, 2, 3);
    graph_add_edge(&g, 2, 0, -6);
    CHECK(!floyd_warshall(&g, all, next));
    graph_free(&g);

    make_nodes(&g, 1);                                  /* one node, no edges */
    CHECK(dijkstra(&g, 0, dist, prev) && dist[0] == 0 && prev[0] == -1);
    CHECK(floyd_warshall(&g, all, next) && all[0] == 0);
    graph_free(&g);
}

/* ---- Dijkstra does no more work than it should ---- */

/* With a correct heap, every reachable node is settled exactly once. A heap
   that pops in the wrong order still ends with the right distances (stale
   entries get corrected later), so only this test notices it. */
static void test_settles_once(void)
{
    char line[64];
    for (int trial = 0; trial < 300; trial++) {
        int n = 2 + (int)rnd(25);
        graph g;
        make_nodes(&g, n);
        for (int k = 0, m = (int)rnd((unsigned)(n * n)); k < m; k++)
            graph_add_edge(&g, (int)rnd((unsigned)n), (int)rnd((unsigned)n), (dist_t)rnd(30));
        dist_t dist[30];
        int prev[30], settled[30] = {0}, reachable = 0;
        FILE *f = tmpfile();
        CHECK(f != NULL);
        if (f == NULL)
            return;
        CHECK(dijkstra_trace(&g, 0, dist, prev, f));
        rewind(f);
        while (fgets(line, sizeof line, f) != NULL) {
            int node;
            if (sscanf(line, "settle %d", &node) == 1 && node >= 0 && node < n)
                settled[node]++;
        }
        fclose(f);
        for (int v = 0; v < n; v++) {
            CHECK(settled[v] == (dist[v] != DIST_INF));
            reachable += dist[v] != DIST_INF;
        }
        CHECK(reachable >= 1);
        graph_free(&g);
    }
}

/* ---- random graphs against Bellman-Ford ---- */

/* Returns false if a negative cycle is reachable from source. */
static bool bellman_ford(const graph *g, int source, dist_t *dist)
{
    for (int v = 0; v < g->n; v++)
        dist[v] = DIST_INF;
    dist[source] = 0;
    for (int round = 0; round <= g->n; round++) {
        bool changed = false;
        for (int u = 0; u < g->n; u++) {
            if (dist[u] == DIST_INF)
                continue;
            for (int k = 0; k < g->deg[u]; k++) {
                const edge *e = &g->adj[u][k];
                if (dist[u] + e->weight < dist[e->to]) {
                    dist[e->to] = dist[u] + e->weight;
                    changed = true;
                }
            }
        }
        if (!changed)
            return true;
    }
    return false;                                       /* still improving after n rounds */
}

/* The path must start and end in the right places, use real edges, and add up. */
static bool path_adds_up(const graph *g, const int *path, int len, int s, int t, dist_t want)
{
    if (len < 1 || path[0] != s || path[len - 1] != t)
        return false;
    dist_t total = 0;
    for (int i = 0; i + 1 < len; i++) {
        dist_t best = DIST_INF;
        for (int k = 0; k < g->deg[path[i]]; k++)
            if (g->adj[path[i]][k].to == path[i + 1] && g->adj[path[i]][k].weight < best)
                best = g->adj[path[i]][k].weight;
        if (best == DIST_INF)
            return false;
        total += best;
    }
    return total == want;
}

enum weights { POSITIVE, WITH_ZERO, NEGATIVE_NO_CYCLE, ANYTHING };

static void random_graph(graph *g, int n, enum weights kind)
{
    make_nodes(g, n);
    int potential[40];
    for (int i = 0; i < n; i++)
        potential[i] = (int)rnd(50);
    int edges = (int)rnd((unsigned)(n * n / 2 + 2));
    for (int k = 0; k < edges; k++) {
        int u = (int)rnd((unsigned)n), v = (int)rnd((unsigned)n);
        dist_t w;
        switch (kind) {
        case POSITIVE: w = 1 + rnd(100); break;
        case WITH_ZERO: w = rnd(4); break;
        /* base + p[u] - p[v]: any cycle's total is the sum of its bases, so never negative */
        case NEGATIVE_NO_CYCLE: w = (dist_t)rnd(20) + potential[u] - potential[v]; break;
        default: w = (dist_t)rnd(40) - 12; break;
        }
        if (u != v || kind != ANYTHING)
            graph_add_edge(g, u, v, w);
    }
}

static void test_random(void)
{
    dist_t bf[40], dist[40], all[1600];
    int prev[40], next[1600], path[40];
    int cycles_found = 0;
    for (int trial = 0; trial < 4000; trial++) {
        enum weights kind = (enum weights)(trial % 4);
        int n = 1 + (int)rnd(30);
        graph g;
        random_graph(&g, n, kind);

        bool has_negative = false;
        for (int u = 0; u < n; u++)
            for (int k = 0; k < g.deg[u]; k++)
                if (g.adj[u][k].weight < 0)
                    has_negative = true;

        bool fw_ok = floyd_warshall(&g, all, next);
        bool any_cycle = false;
        for (int s = 0; s < n; s++)
            if (!bellman_ford(&g, s, bf))
                any_cycle = true;
        CHECK(fw_ok == !any_cycle);
        cycles_found += any_cycle;

        for (int s = 0; s < n && fw_ok; s++) {
            bellman_ford(&g, s, bf);
            bool dj_ok = dijkstra(&g, s, dist, prev);
            CHECK(dj_ok == !has_negative);              /* refuses exactly when it must */
            for (int t = 0; t < n; t++) {
                CHECK(all[s * n + t] == bf[t]);
                if (dj_ok)
                    CHECK(dist[t] == bf[t]);
                if (bf[t] == DIST_INF) {
                    CHECK(path_from_next(next, n, s, t, path, n) == 0);
                    continue;
                }
                int len = path_from_next(next, n, s, t, path, n);
                CHECK(path_adds_up(&g, path, len, s, t, bf[t]));
                if (dj_ok) {
                    len = path_from_prev(prev, s, t, path, n);
                    CHECK(path_adds_up(&g, path, len, s, t, bf[t]));
                }
            }
        }
        graph_free(&g);
    }
    CHECK(cycles_found > 100);                          /* the ANYTHING graphs really do have cycles */
}

int main(int argc, char **argv)
{
    test_loader();
    test_lakeland(argc > 1 ? argv[1] : "data/lakeland.txt");
    test_special_edges();
    test_settles_once();
    test_random();
    if (failures) {
        printf("%d of %d checks failed\n", failures, checks);
        return 1;
    }
    printf("all %d checks passed\n", checks);
    return 0;
}
