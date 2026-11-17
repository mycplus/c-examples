#include "shortest_path.h"

#include <stdlib.h>

/* A min-heap of (distance, node) pairs. A node may be pushed more than once;
   stale entries are skipped when popped ("lazy deletion"). */
typedef struct {
    dist_t d;
    int node;
} heap_item;

typedef struct {
    heap_item *items;
    size_t len, cap;
} heap;

static bool heap_push(heap *h, dist_t d, int node)
{
    if (h->len == h->cap) {
        size_t cap = h->cap ? h->cap * 2 : 16;
        heap_item *bigger = realloc(h->items, cap * sizeof *bigger);
        if (bigger == NULL)
            return false;
        h->items = bigger;
        h->cap = cap;
    }
    size_t i = h->len++;
    while (i > 0 && h->items[(i - 1) / 2].d > d) {     /* sift up */
        h->items[i] = h->items[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    h->items[i].d = d;
    h->items[i].node = node;
    return true;
}

static heap_item heap_pop(heap *h)
{
    heap_item top = h->items[0], last = h->items[--h->len];
    size_t i = 0;
    for (;;) {                                          /* sift down */
        size_t child = 2 * i + 1;
        if (child >= h->len)
            break;
        if (child + 1 < h->len && h->items[child + 1].d < h->items[child].d)
            child++;
        if (h->items[child].d >= last.d)
            break;
        h->items[i] = h->items[child];
        i = child;
    }
    if (h->len > 0)
        h->items[i] = last;
    return top;
}

bool dijkstra(const graph *g, int source, dist_t *dist, int *prev)
{
    return dijkstra_trace(g, source, dist, prev, NULL);
}

bool dijkstra_trace(const graph *g, int source, dist_t *dist, int *prev, FILE *trace)
{
    for (int u = 0; u < g->n; u++)
        for (int k = 0; k < g->deg[u]; k++)
            if (g->adj[u][k].weight < 0)
                return false;

    for (int v = 0; v < g->n; v++) {
        dist[v] = DIST_INF;
        prev[v] = -1;
    }
    dist[source] = 0;

    heap h = {NULL, 0, 0};
    bool ok = heap_push(&h, 0, source);
    while (ok && h.len > 0) {
        heap_item top = heap_pop(&h);
        if (top.d > dist[top.node])
            continue;                                   /* a stale entry */
        int u = top.node;
        if (trace)
            fprintf(trace, "settle %-12s at %lld\n", g->names[u], top.d);
        for (int k = 0; k < g->deg[u]; k++) {
            const edge *e = &g->adj[u][k];
            dist_t candidate = dist[u] + e->weight;
            if (candidate < dist[e->to]) {
                dist[e->to] = candidate;
                prev[e->to] = u;
                if (trace)
                    fprintf(trace, "    %-12s %lld via %s\n", g->names[e->to], candidate, g->names[u]);
                ok = heap_push(&h, candidate, e->to);
                if (!ok)
                    break;
            }
        }
    }
    free(h.items);
    return ok;
}

bool floyd_warshall(const graph *g, dist_t *dist, int *next)
{
    return floyd_warshall_trace(g, dist, next, NULL);
}

static void print_matrix(const graph *g, const dist_t *dist, FILE *out)
{
    fprintf(out, "%-10s", "");
    for (int j = 0; j < g->n; j++)
        fprintf(out, "%8.7s", g->names[j]);
    fputc('\n', out);
    for (int i = 0; i < g->n; i++) {
        fprintf(out, "%-10.9s", g->names[i]);
        for (int j = 0; j < g->n; j++) {
            if (dist[i * g->n + j] == DIST_INF)
                fprintf(out, "%8s", "-");
            else
                fprintf(out, "%8lld", dist[i * g->n + j]);
        }
        fputc('\n', out);
    }
}

bool floyd_warshall_trace(const graph *g, dist_t *dist, int *next, FILE *trace)
{
    const int n = g->n;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            dist[i * n + j] = i == j ? 0 : DIST_INF;
            next[i * n + j] = i == j ? i : -1;
        }
    for (int u = 0; u < n; u++)
        for (int k = 0; k < g->deg[u]; k++) {
            const edge *e = &g->adj[u][k];
            if (e->weight < dist[u * n + e->to]) {      /* keep the shortest parallel road */
                dist[u * n + e->to] = e->weight;
                next[u * n + e->to] = e->to;
            }
        }

    if (trace) {
        fprintf(trace, "Start: direct roads only\n");
        print_matrix(g, dist, trace);
    }
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            if (dist[i * n + k] == DIST_INF)
                continue;                               /* never add to "infinity" */
            for (int j = 0; j < n; j++) {
                if (dist[k * n + j] == DIST_INF)
                    continue;
                dist_t through_k = dist[i * n + k] + dist[k * n + j];
                if (through_k < dist[i * n + j]) {
                    dist[i * n + j] = through_k;
                    next[i * n + j] = next[i * n + k];
                }
            }
        }
        if (trace) {
            fprintf(trace, "\nAfter allowing stops at %s (k = %d)\n", g->names[k], k);
            print_matrix(g, dist, trace);
        }
        /* A town that can reach itself at negative cost lies on a negative cycle.
           Stop now: every further round would push the totals further down,
           doubling them, until they overflow. */
        for (int i = 0; i < n; i++)
            if (dist[i * n + i] < 0)
                return false;
    }
    return true;
}

int path_from_prev(const int *prev, int source, int target, int *out, int cap)
{
    int len = 0;
    for (int v = target; v != -1; v = prev[v]) {
        if (len == cap)
            return 0;
        out[len++] = v;
    }
    if (out[len - 1] != source)
        return 0;                                       /* the chain never reached the source */
    for (int i = 0, j = len - 1; i < j; i++, j--) {     /* it was built backwards */
        int t = out[i];
        out[i] = out[j];
        out[j] = t;
    }
    return len;
}

int path_from_next(const int *next, int n, int source, int target, int *out, int cap)
{
    if (next[source * n + target] == -1)
        return 0;
    int len = 0;
    for (int v = source; ; v = next[v * n + target]) {
        if (len == cap)
            return 0;
        out[len++] = v;
        if (v == target)
            return len;
    }
}
