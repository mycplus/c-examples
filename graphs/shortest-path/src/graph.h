#ifndef GRAPH_H
#define GRAPH_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/* Distances are 64-bit, and "no path" is a separate value that is never added to. */
typedef long long dist_t;
#define DIST_INF LLONG_MAX

typedef struct {
    int to;
    dist_t weight;
} edge;

/* A weighted directed graph stored as adjacency lists. An undirected road is
   two edges, one in each direction. Nodes have names, so maps can use towns. */
typedef struct {
    int n;               /* number of nodes */
    int cap;
    char **names;
    edge **adj;          /* adj[u][0 .. deg[u]-1] are the edges leaving u */
    int *deg;
    int *adj_cap;
} graph;

void graph_init(graph *g);
void graph_free(graph *g);

/* Returns the node's index, adding it if the name is new; -1 if out of memory. */
int graph_add_node(graph *g, const char *name);
/* Returns the node's index, or -1 if there is no node with that name. */
int graph_find(const graph *g, const char *name);
bool graph_add_edge(graph *g, int from, int to, dist_t weight);

/* Reads "Town A,Town B,distance" lines (undirected roads). Blank lines and
   lines starting with '#' are skipped. On error, fills err with the line
   number and the reason, and returns false. */
bool graph_load_map(graph *g, FILE *in, char *err, size_t err_len);

#endif
