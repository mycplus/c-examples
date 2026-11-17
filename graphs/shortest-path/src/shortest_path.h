#ifndef SHORTEST_PATH_H
#define SHORTEST_PATH_H

#include "graph.h"

/* Dijkstra's algorithm from one source, with a binary heap.
   dist[v] is the shortest distance to v (DIST_INF if unreachable) and prev[v]
   the node before v on that path (-1 for the source and unreachable nodes).
   Returns false, without searching, if any edge weight is negative:
   Dijkstra's algorithm is only correct for non-negative weights. */
bool dijkstra(const graph *g, int source, dist_t *dist, int *prev);

/* The same, printing each step to trace (used by the walkthrough program). */
bool dijkstra_trace(const graph *g, int source, dist_t *dist, int *prev, FILE *trace);

/* Floyd-Warshall: every pair at once. dist and next are n*n arrays, row-major.
   next[i*n + j] is the first node after i on the shortest path to j (-1 if none).
   Returns false if the graph has a negative cycle, which makes "shortest" meaningless. */
bool floyd_warshall(const graph *g, dist_t *dist, int *next);

/* The same, printing the distance matrix after each round k to trace. */
bool floyd_warshall_trace(const graph *g, dist_t *dist, int *next, FILE *trace);

/* Rebuilds a path into out[0 .. returned-1], source first. Returns 0 if there is no path. */
int path_from_prev(const int *prev, int source, int target, int *out, int cap);
int path_from_next(const int *next, int n, int source, int target, int *out, int cap);

#endif
