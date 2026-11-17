#include "graph.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

void graph_init(graph *g)
{
    memset(g, 0, sizeof *g);
}

void graph_free(graph *g)
{
    for (int i = 0; i < g->n; i++) {
        free(g->names[i]);
        free(g->adj[i]);
    }
    free(g->names);
    free(g->adj);
    free(g->deg);
    free(g->adj_cap);
    graph_init(g);
}

int graph_find(const graph *g, const char *name)
{
    for (int i = 0; i < g->n; i++)
        if (strcmp(g->names[i], name) == 0)
            return i;
    return -1;
}

int graph_add_node(graph *g, const char *name)
{
    int found = graph_find(g, name);
    if (found >= 0)
        return found;
    if (g->n == g->cap) {
        size_t cap = g->cap ? (size_t)g->cap * 2 : 8;
        char **names = realloc(g->names, cap * sizeof *names);
        if (names == NULL)
            return -1;
        g->names = names;
        edge **adj = realloc(g->adj, cap * sizeof *adj);
        if (adj == NULL)
            return -1;
        g->adj = adj;
        int *deg = realloc(g->deg, cap * sizeof *deg);
        if (deg == NULL)
            return -1;
        g->deg = deg;
        int *adj_cap = realloc(g->adj_cap, cap * sizeof *adj_cap);
        if (adj_cap == NULL)
            return -1;
        g->adj_cap = adj_cap;
        g->cap = (int)cap;
    }
    size_t len = strlen(name);
    char *copy = malloc(len + 1);
    if (copy == NULL)
        return -1;
    memcpy(copy, name, len + 1);
    g->names[g->n] = copy;
    g->adj[g->n] = NULL;
    g->deg[g->n] = 0;
    g->adj_cap[g->n] = 0;
    return g->n++;
}

bool graph_add_edge(graph *g, int from, int to, dist_t weight)
{
    if (from < 0 || from >= g->n || to < 0 || to >= g->n)
        return false;
    if (g->deg[from] == g->adj_cap[from]) {
        size_t cap = g->adj_cap[from] ? (size_t)g->adj_cap[from] * 2 : 4;
        edge *bigger = realloc(g->adj[from], cap * sizeof *bigger);
        if (bigger == NULL)
            return false;
        g->adj[from] = bigger;
        g->adj_cap[from] = (int)cap;
    }
    edge *e = &g->adj[from][g->deg[from]++];
    e->to = to;
    e->weight = weight;
    return true;
}

/* Removes leading and trailing spaces in place. */
static char *trim(char *s)
{
    while (isspace((unsigned char)*s))
        s++;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1]))
        *--end = '\0';
    return s;
}

bool graph_load_map(graph *g, FILE *in, char *err, size_t err_len)
{
    char line[256];
    int line_no = 0;
    while (fgets(line, sizeof line, in) != NULL) {
        line_no++;
        if (strchr(line, '\n') == NULL && !feof(in)) {
            snprintf(err, err_len, "line %d: longer than %zu characters", line_no, sizeof line - 2);
            return false;
        }
        char *text = trim(line);
        if (*text == '\0' || *text == '#')
            continue;

        char *a = text, *b = strchr(a, ','), *d = b ? strchr(b + 1, ',') : NULL;
        if (b == NULL || d == NULL || strchr(d + 1, ',') != NULL) {
            snprintf(err, err_len, "line %d: expected \"town,town,distance\"", line_no);
            return false;
        }
        *b++ = '\0';
        *d++ = '\0';
        a = trim(a);
        b = trim(b);
        d = trim(d);

        char *end;
        errno = 0;
        long long km = strtoll(d, &end, 10);
        if (*a == '\0' || *b == '\0' || end == d || *end != '\0' || errno == ERANGE ||
            km <= 0 || km > 1000000000LL) {
            snprintf(err, err_len, "line %d: needs two town names and a distance from 1 to 1000000000", line_no);
            return false;
        }
        if (strcmp(a, b) == 0) {
            snprintf(err, err_len, "line %d: a road from \"%s\" to itself", line_no, a);
            return false;
        }
        int u = graph_add_node(g, a), v = graph_add_node(g, b);
        if (u < 0 || v < 0 || !graph_add_edge(g, u, v, km) || !graph_add_edge(g, v, u, km)) {
            snprintf(err, err_len, "out of memory");
            return false;
        }
    }
    if (g->n == 0) {
        snprintf(err, err_len, "the map has no roads");
        return false;
    }
    return true;
}
