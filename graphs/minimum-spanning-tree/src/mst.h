/* mst.h - the minimum spanning tree problem, broken into pieces:
 * an edge list, a sort, a disjoint-set (union-find) structure, Kruskal's
 * greedy loop, and a brute force to check it against. */
#ifndef MST_H
#define MST_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int u, v;      /* the two vertices, numbered from 0 */
    int weight;    /* the cost of the edge */
} Edge;

/* Disjoint sets over the vertices 0 .. n-1: which vertices are already
 * connected to each other by the edges chosen so far. */
typedef struct {
    int *parent;
    int *size;
    int n;
} DisjointSet;

bool ds_init(DisjointSet *ds, int n);
void ds_free(DisjointSet *ds);
int ds_find(DisjointSet *ds, int x);              /* the set's representative */
bool ds_union(DisjointSet *ds, int a, int b);     /* false if already joined */

/* Sorts edges by weight, cheapest first; equal weights keep a fixed order. */
void sort_edges(Edge *edges, size_t m);

/* Kruskal's algorithm. Sorts `edges` in place, writes the n - 1 chosen edges
 * to `tree` and their total weight to *cost. Returns false if the graph is
 * not connected (or memory runs out). Weights may be negative, so no cost
 * value is free to mean "no tree". */
bool kruskal(int n, Edge *edges, size_t m, Edge *tree, long long *cost);

/* The simplest correct solution: try every set of n - 1 edges and keep the
 * cheapest one that connects all n vertices. Writes its cost to *cost and the
 * number of sets examined to *sets_checked; returns false if no set connects
 * the graph. Exponential: for small graphs and tests only. */
bool mst_brute_force(int n, const Edge *edges, size_t m, long long *cost,
                     unsigned long long *sets_checked);

#endif
