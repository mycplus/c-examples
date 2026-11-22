/* mst_speed.c - where Kruskal's algorithm spends its time: sorting the
 * edges, then the union-find loop, on random connected graphs with four
 * edges per vertex. Median of 5 runs. Not run by ctest. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mst.h"

static Edge make_edge(int u, int v, int weight)
{
    Edge e;
    e.u = u;
    e.v = v;
    e.weight = weight;
    return e;
}

static double now_ms(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1e3 + (double)ts.tv_nsec / 1e6;
}

static int cmp_double(const void *x, const void *y)
{
    double a = *(const double *)x, b = *(const double *)y;
    return (a > b) - (a < b);
}

int main(void)
{
    const int sizes[] = {10000, 100000, 1000000};
    printf("%9s %9s %10s %16s %12s\n", "vertices", "edges", "sort ms", "union-find ms", "sort share");
    for (size_t s = 0; s < sizeof sizes / sizeof sizes[0]; s++) {
        int n = sizes[s];
        size_t m = 4 * (size_t)n;
        Edge *source = malloc(m * sizeof *source), *e = malloc(m * sizeof *e);
        if (source == NULL || e == NULL)
            return 1;
        uint32_t r = 12345;
        for (size_t k = 0; k < m; k++) {
            r = r * 1664525u + 1013904223u;
            int u = k < (size_t)n - 1 ? (int)k + 1 : (int)(r % (uint32_t)n);
            r = r * 1664525u + 1013904223u;
            int v = k < (size_t)n - 1 ? (int)(r % (uint32_t)(k + 1)) : (int)(r % (uint32_t)n);
            r = r * 1664525u + 1013904223u;
            source[k] = make_edge(u, v, (int)(r >> 8) % 1000000);
        }
        double sort_t[5], loop_t[5];
        long long check = 0;
        for (int run = 0; run < 5; run++) {
            memcpy(e, source, m * sizeof *e);
            double t0 = now_ms();
            sort_edges(e, m);
            double t1 = now_ms();
            DisjointSet ds;
            if (!ds_init(&ds, n))
                return 1;
            long long cost = 0;
            int taken = 0;
            for (size_t k = 0; k < m && taken < n - 1; k++)
                if (ds_union(&ds, e[k].u, e[k].v)) {
                    taken++;
                    cost += e[k].weight;
                }
            ds_free(&ds);
            double t2 = now_ms();
            if (taken != n - 1 || (run > 0 && cost != check)) {
                fprintf(stderr, "unexpected result\n");
                return 1;
            }
            check = cost;
            sort_t[run] = t1 - t0;
            loop_t[run] = t2 - t1;
        }
        qsort(sort_t, 5, sizeof sort_t[0], cmp_double);
        qsort(loop_t, 5, sizeof loop_t[0], cmp_double);
        printf("%9d %9zu %10.1f %16.1f %11.0f%%\n", n, m, sort_t[2], loop_t[2],
               100 * sort_t[2] / (sort_t[2] + loop_t[2]));
        free(source);
        free(e);
    }
    return 0;
}
