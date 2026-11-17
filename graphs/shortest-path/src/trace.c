/* trace: shows each step of the two algorithms on a map file.

   trace MAP dijkstra FROM   the order towns are settled, and every improvement
   trace MAP floyd           the distance table after each round of Floyd-Warshall */
#include "graph.h"
#include "shortest_path.h"

#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    bool want_dijkstra = argc == 4 && strcmp(argv[2], "dijkstra") == 0;
    bool want_floyd = argc == 3 && strcmp(argv[2], "floyd") == 0;
    if (!want_dijkstra && !want_floyd) {
        fprintf(stderr, "usage: trace MAP dijkstra FROM | trace MAP floyd\n");
        return 2;
    }
    FILE *in = fopen(argv[1], "r");
    if (in == NULL) {
        fprintf(stderr, "trace: cannot open %s\n", argv[1]);
        return 2;
    }
    graph g;
    graph_init(&g);
    char err[128];
    bool loaded = graph_load_map(&g, in, err, sizeof err);
    fclose(in);
    if (!loaded) {
        fprintf(stderr, "trace: %s\n", err);
        graph_free(&g);
        return 2;
    }

    int status = 0, n = g.n;
    if (want_dijkstra) {
        int s = graph_find(&g, argv[3]);
        dist_t *dist = malloc((size_t)n * sizeof *dist);
        int *prev = malloc((size_t)n * sizeof *prev);
        if (s < 0 || dist == NULL || prev == NULL || !dijkstra_trace(&g, s, dist, prev, stdout))
            status = 2;
        free(dist);
        free(prev);
    } else {
        dist_t *dist = malloc((size_t)n * (size_t)n * sizeof *dist);
        int *next = malloc((size_t)n * (size_t)n * sizeof *next);
        if (dist == NULL || next == NULL || !floyd_warshall_trace(&g, dist, next, stdout))
            status = 2;
        free(dist);
        free(next);
    }
    if (status != 0)
        fprintf(stderr, "trace: failed (unknown town?)\n");
    graph_free(&g);
    return status;
}
