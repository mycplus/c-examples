/* route: find the shortest road route between two towns on a map file.

   route MAP FROM TO [dijkstra|floyd]   one route, with a table of legs
   route MAP --from TOWN                 distance from TOWN to every town
   route MAP --table                     the all-pairs distance table

   Exit status: 0 route found, 1 no route exists, 2 bad input. */
#include "graph.h"
#include "shortest_path.h"

#include <stdlib.h>
#include <string.h>

static int name_width(const graph *g)
{
    int w = 4;
    for (int i = 0; i < g->n; i++) {
        int len = (int)strlen(g->names[i]);
        if (len > w)
            w = len;
    }
    return w;
}

static int find_town(const graph *g, const char *name)
{
    int i = graph_find(g, name);
    if (i < 0)
        fprintf(stderr, "route: unknown town \"%s\"\n", name);
    return i;
}

/* Prints the legs of a path, looking each road's length up in the graph. */
static void print_route(const graph *g, const int *path, int len, const char *method)
{
    int w = name_width(g);
    printf("%s to %s (%s)\n\n", g->names[path[0]], g->names[path[len - 1]], method);
    printf("  %-*s  %-*s  %7s  %8s\n", w, "From", w, "To", "Road km", "Total km");
    dist_t total = 0;
    for (int i = 0; i + 1 < len; i++) {
        dist_t road = DIST_INF;
        for (int k = 0; k < g->deg[path[i]]; k++) {
            const edge *e = &g->adj[path[i]][k];
            if (e->to == path[i + 1] && e->weight < road)
                road = e->weight;
        }
        total += road;
        printf("  %-*s  %-*s  %7lld  %8lld\n", w, g->names[path[i]], w, g->names[path[i + 1]], road, total);
    }
    printf("\nTotal: %lld km over %d road%s\n", total, len - 1, len == 2 ? "" : "s");
}

static int one_route(const graph *g, const char *from, const char *to, const char *method)
{
    int s = find_town(g, from), t = find_town(g, to);
    if (s < 0 || t < 0)
        return 2;
    bool use_floyd = strcmp(method, "floyd") == 0;
    if (!use_floyd && strcmp(method, "dijkstra") != 0) {
        fprintf(stderr, "route: method must be dijkstra or floyd\n");
        return 2;
    }

    int n = g->n, *path = malloc((size_t)n * sizeof *path), len = 0;
    if (path == NULL)
        return 2;
    int status = 0;
    if (use_floyd) {
        dist_t *dist = malloc((size_t)n * (size_t)n * sizeof *dist);
        int *next = malloc((size_t)n * (size_t)n * sizeof *next);
        if (dist == NULL || next == NULL || !floyd_warshall(g, dist, next))
            status = 2;
        else
            len = path_from_next(next, n, s, t, path, n);
        free(dist);
        free(next);
    } else {
        dist_t *dist = malloc((size_t)n * sizeof *dist);
        int *prev = malloc((size_t)n * sizeof *prev);
        if (dist == NULL || prev == NULL || !dijkstra(g, s, dist, prev))
            status = 2;
        else
            len = path_from_prev(prev, s, t, path, n);
        free(dist);
        free(prev);
    }

    if (status == 0 && len == 0) {
        printf("No route from %s to %s: no chain of roads connects them.\n", from, to);
        status = 1;
    } else if (status == 0) {
        print_route(g, path, len, use_floyd ? "Floyd-Warshall" : "Dijkstra");
    } else {
        fprintf(stderr, "route: search failed\n");
    }
    free(path);
    return status;
}

static int from_town(const graph *g, const char *from)
{
    int s = find_town(g, from);
    if (s < 0)
        return 2;
    int n = g->n, w = name_width(g);
    dist_t *dist = malloc((size_t)n * sizeof *dist);
    int *prev = malloc((size_t)n * sizeof *prev);
    int status = 0;
    if (dist == NULL || prev == NULL || !dijkstra(g, s, dist, prev)) {
        fprintf(stderr, "route: search failed\n");
        status = 2;
    } else {
        printf("Distances from %s (Dijkstra)\n\n", g->names[s]);
        for (int v = 0; v < n; v++) {
            if (dist[v] == DIST_INF)
                printf("  %-*s  no route\n", w, g->names[v]);
            else
                printf("  %-*s  %5lld km  via %s\n", w, g->names[v], dist[v],
                       prev[v] < 0 ? "-" : g->names[prev[v]]);
        }
    }
    free(dist);
    free(prev);
    return status;
}

static int table(const graph *g)
{
    int n = g->n;
    dist_t *dist = malloc((size_t)n * (size_t)n * sizeof *dist);
    int *next = malloc((size_t)n * (size_t)n * sizeof *next);
    int status = 0;
    if (dist == NULL || next == NULL || !floyd_warshall(g, dist, next)) {
        fprintf(stderr, "route: search failed\n");
        status = 2;
    } else {
        printf("%-6s", "km");
        for (int j = 0; j < n; j++)
            printf("%6.4s", g->names[j]);
        putchar('\n');
        for (int i = 0; i < n; i++) {
            printf("%-6.4s", g->names[i]);
            for (int j = 0; j < n; j++) {
                if (dist[i * n + j] == DIST_INF)
                    printf("%6s", "-");
                else
                    printf("%6lld", dist[i * n + j]);
            }
            putchar('\n');
        }
    }
    free(dist);
    free(next);
    return status;
}

int main(int argc, char **argv)
{
    if (argc < 3 || argc > 5) {
        fprintf(stderr, "usage: route MAP FROM TO [dijkstra|floyd]\n"
                        "       route MAP --from TOWN\n"
                        "       route MAP --table\n");
        return 2;
    }
    FILE *in = fopen(argv[1], "r");
    if (in == NULL) {
        fprintf(stderr, "route: cannot open %s\n", argv[1]);
        return 2;
    }
    graph g;
    graph_init(&g);
    char err[128];
    bool loaded = graph_load_map(&g, in, err, sizeof err);
    fclose(in);
    if (!loaded) {
        fprintf(stderr, "route: %s: %s\n", argv[1], err);
        graph_free(&g);
        return 2;
    }

    int status;
    if (strcmp(argv[2], "--table") == 0 && argc == 3)
        status = table(&g);
    else if (strcmp(argv[2], "--from") == 0 && argc == 4)
        status = from_town(&g, argv[3]);
    else if (argc >= 4 && argv[2][0] != '-')
        status = one_route(&g, argv[2], argv[3], argc == 5 ? argv[4] : "dijkstra");
    else {
        fprintf(stderr, "route: bad arguments (run with no arguments for usage)\n");
        status = 2;
    }
    graph_free(&g);
    return status;
}
