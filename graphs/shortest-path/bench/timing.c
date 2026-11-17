/* timing: when does running Dijkstra from every node beat Floyd-Warshall?

   For each graph size, builds a sparse "road network" (every node has about
   4 roads, like a real map) and a dense graph (every pair connected), then
   times all-pairs shortest paths both ways:
     - Dijkstra from every source: n runs of O((V + E) log V)
     - Floyd-Warshall: one run of O(V^3)
   Both must produce the same checksum (the sum of all finite distances),
   or the program stops: a timing of two different answers means nothing.

   usage: timing [largest n]   (default 800; sizes double from 100) */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 199309L                         /* clock_gettime */
#endif
#include "graph.h"
#include "shortest_path.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define RUNS 5

static unsigned long long rng_state = 0x2545F4914F6CDD1DULL;
static unsigned long long rng(void)                    /* xorshift64 */
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}

static bool build(graph *g, int n, bool dense)
{
    char name[16];
    rng_state = 0x2545F4914F6CDD1DULL + 2 * (unsigned long long)n + dense;   /* same graph every run */
    for (int i = 0; i < n; i++) {
        snprintf(name, sizeof name, "n%d", i);
        if (graph_add_node(g, name) != i)
            return false;
    }
    if (dense) {
        for (int u = 0; u < n; u++)
            for (int v = 0; v < n; v++)
                if (u != v && !graph_add_edge(g, u, v, 1 + (dist_t)(rng() % 1000)))
                    return false;
        return true;
    }
    for (int u = 0; u < n; u++) {                       /* a ring keeps it connected */
        dist_t w = 1 + (dist_t)(rng() % 1000);
        int v = (u + 1) % n;
        if (!graph_add_edge(g, u, v, w) || !graph_add_edge(g, v, u, w))
            return false;
    }
    for (int k = 0; k < n; k++) {                       /* plus one random road per node */
        int u = (int)(rng() % (unsigned)n), v = (int)(rng() % (unsigned)n);
        dist_t w = 1 + (dist_t)(rng() % 1000);
        if (u != v && (!graph_add_edge(g, u, v, w) || !graph_add_edge(g, v, u, w)))
            return false;
    }
    return true;
}

/* A monotonic clock in milliseconds: it never jumps when the system time is set. */
static double now_ms(void)
{
#ifdef _WIN32
    static LARGE_INTEGER freq;
    LARGE_INTEGER t;
    if (freq.QuadPart == 0)
        QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart * 1e3 / (double)freq.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e3 + (double)ts.tv_nsec / 1e6;
#endif
}

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static double median(double *t)
{
    qsort(t, RUNS, sizeof *t, cmp_double);
    return t[RUNS / 2];
}

static long long checksum(const dist_t *dist, size_t count)
{
    long long sum = 0;
    for (size_t i = 0; i < count; i++)
        if (dist[i] != DIST_INF)
            sum += dist[i];
    return sum;
}

int main(int argc, char **argv)
{
    int largest = argc > 1 ? atoi(argv[1]) : 800;
    printf("%-7s %6s %8s %12s %14s %14s %14s  %s\n", "Graph", "Nodes", "Edges",
           "Dijkstra 1", "Dijkstra all", "Floyd-Warshall", "Checksum", "All pairs: faster");
    printf("%-7s %6s %8s %12s %14s %14s\n", "", "", "", "(ms)", "(ms)", "(ms)");

    for (int dense = 0; dense <= 1; dense++)
        for (int n = 100; n <= largest; n *= 2) {
            graph g;
            graph_init(&g);
            size_t cells = (size_t)n * (size_t)n;
            dist_t *all = malloc(cells * sizeof *all), *fw = malloc(cells * sizeof *fw);
            int *prev = malloc((size_t)n * sizeof *prev), *next = malloc(cells * sizeof *next);
            if (!build(&g, n, dense) || !all || !fw || !prev || !next) {
                fprintf(stderr, "timing: out of memory\n");
                return 1;
            }
            long edges = 0;
            for (int u = 0; u < n; u++)
                edges += g.deg[u];

            double td[RUNS], tf[RUNS];
            for (int r = 0; r < RUNS; r++) {
                double t0 = now_ms();
                for (int s = 0; s < n; s++)
                    if (!dijkstra(&g, s, all + (size_t)s * (size_t)n, prev))
                        return 1;
                double t1 = now_ms();
                if (!floyd_warshall(&g, fw, next))
                    return 1;
                double t2 = now_ms();
                td[r] = t1 - t0;
                tf[r] = t2 - t1;
            }
            long long a = checksum(all, cells), b = checksum(fw, cells);
            if (a != b) {
                fprintf(stderr, "timing: checksums differ (%lld vs %lld)\n", a, b);
                return 1;
            }
            double d = median(td), f = median(tf);
            printf("%-7s %6d %8ld %12.3f %14.2f %14.2f %14lld  %s %.1fx\n", dense ? "dense" : "sparse",
                   n, edges, d / n, d, f, a, d < f ? "Dijkstra" : "Floyd-Warshall", d < f ? f / d : d / f);
            free(all);
            free(fw);
            free(prev);
            free(next);
            graph_free(&g);
        }
    printf("\nMedian of %d runs. \"Dijkstra 1\" is the all-sources time divided by n.\n", RUNS);
    return 0;
}
