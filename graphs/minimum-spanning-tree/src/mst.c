/* mst.c - Kruskal's algorithm, built from small pieces. */
#include "mst.h"

#include <stdlib.h>

/* ---- Piece 1: disjoint sets (union-find) --------------------------------- */

bool ds_init(DisjointSet *ds, int n)
{
    ds->n = n;
    ds->parent = malloc((size_t)n * sizeof *ds->parent);
    ds->size = malloc((size_t)n * sizeof *ds->size);
    if (n > 0 && (ds->parent == NULL || ds->size == NULL)) {
        ds_free(ds);
        return false;
    }
    for (int i = 0; i < n; i++) {
        ds->parent[i] = i;      /* every vertex starts in a set of its own */
        ds->size[i] = 1;
    }
    return true;
}

void ds_free(DisjointSet *ds)
{
    free(ds->parent);
    free(ds->size);
    ds->parent = ds->size = NULL;
}

int ds_find(DisjointSet *ds, int x)
{
    while (ds->parent[x] != x) {
        ds->parent[x] = ds->parent[ds->parent[x]];   /* path halving */
        x = ds->parent[x];
    }
    return x;
}

bool ds_union(DisjointSet *ds, int a, int b)
{
    a = ds_find(ds, a);
    b = ds_find(ds, b);
    if (a == b)
        return false;                    /* already connected: a cycle */
    if (ds->size[a] < ds->size[b]) {     /* hang the smaller tree below */
        int t = a;
        a = b;
        b = t;
    }
    ds->parent[b] = a;
    ds->size[a] += ds->size[b];
    return true;
}

/* ---- Piece 2: sorting the edges ----------------------------------------- */

static int by_weight(const void *x, const void *y)
{
    const Edge *a = x, *b = y;
    if (a->weight != b->weight)
        return (a->weight > b->weight) - (a->weight < b->weight);
    if (a->u != b->u)                    /* ties: a fixed order, so the */
        return (a->u > b->u) - (a->u < b->u);   /* result is repeatable */
    return (a->v > b->v) - (a->v < b->v);
}

void sort_edges(Edge *edges, size_t m)
{
    qsort(edges, m, sizeof *edges, by_weight);
}

/* ---- Piece 3: the greedy loop ------------------------------------------- */

bool kruskal(int n, Edge *edges, size_t m, Edge *tree, long long *cost)
{
    DisjointSet ds;
    *cost = 0;
    if (n <= 1)
        return n == 0 || n == 1;         /* nothing to join */
    if (!ds_init(&ds, n))
        return false;
    sort_edges(edges, m);
    int taken = 0;
    for (size_t i = 0; i < m && taken < n - 1; i++) {
        if (ds_union(&ds, edges[i].u, edges[i].v)) {   /* joins two parts */
            tree[taken++] = edges[i];
            *cost += edges[i].weight;
        }
    }
    ds_free(&ds);
    return taken == n - 1;
}

/* ---- The brute force ----------------------------------------------------- */

/* Tries every way to choose the remaining `need` edges from edges[from..m). */
static void choose(int n, const Edge *edges, size_t m, size_t from, int need,
                   const Edge **chosen, int count, bool *found, long long *best,
                   unsigned long long *sets)
{
    if (need == 0) {
        (*sets)++;
        DisjointSet ds;
        if (!ds_init(&ds, n))
            return;
        long long total = 0;
        bool tree = true;
        for (int k = 0; k < count && tree; k++) {
            tree = ds_union(&ds, chosen[k]->u, chosen[k]->v);   /* no cycle */
            total += chosen[k]->weight;
        }
        ds_free(&ds);
        if (tree && (!*found || total < *best)) {
            *found = true;
            *best = total;
        }
        return;
    }
    for (size_t i = from; i + (size_t)need <= m; i++) {
        chosen[count] = &edges[i];
        choose(n, edges, m, i + 1, need - 1, chosen, count + 1, found, best, sets);
    }
}

bool mst_brute_force(int n, const Edge *edges, size_t m, long long *cost,
                     unsigned long long *sets_checked)
{
    bool found = false;
    *cost = 0;
    *sets_checked = 0;
    if (n <= 1)
        return n == 0 || n == 1;         /* nothing to join */
    const Edge **chosen = malloc((size_t)(n - 1) * sizeof *chosen);
    if (chosen == NULL)
        return false;
    choose(n, edges, m, 0, n - 1, chosen, 0, &found, cost, sets_checked);
    free(chosen);
    return found;
}
