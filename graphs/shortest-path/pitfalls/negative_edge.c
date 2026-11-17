/* Pitfall: Dijkstra's algorithm with a negative edge weight.

   An electric delivery van's route planner measures each road in watt-hours
   of battery used. Going downhill, regenerative braking puts energy back,
   so that road has a negative cost. Roads are one-way here because a climb
   and a descent cost different amounts.

   The textbook Dijkstra below settles each place once and never revisits it.
   That is only correct when no edge is negative. */
#include <stdbool.h>
#include <stdio.h>

enum { DEPOT, RIDGE, VALLEY, TOWN, N };
static const char *const names[N] = {"Depot", "Ridge", "Valley", "Town"};

/* cost[u][v] in Wh; 0 means no road (no road here costs exactly 0 Wh). */
static const long long cost[N][N] = {
    /*            Depot Ridge Valley Town */
    /* Depot  */ {0,    800,  500,   0},
    /* Ridge  */ {0,    0,    -400,  0},     /* downhill: the battery gains 400 Wh */
    /* Valley */ {0,    0,    0,     300},
    /* Town   */ {0,    0,    0,     0},
};
#define NONE (-1LL)

static void textbook_dijkstra(long long dist[N])
{
    bool settled[N] = {false};
    for (int v = 0; v < N; v++)
        dist[v] = NONE;
    dist[DEPOT] = 0;
    for (int round = 0; round < N; round++) {
        int u = -1;
        for (int v = 0; v < N; v++)                     /* closest unsettled place */
            if (!settled[v] && dist[v] != NONE && (u < 0 || dist[v] < dist[u]))
                u = v;
        if (u < 0)
            break;
        settled[u] = true;                              /* final, or so it assumes */
        for (int v = 0; v < N; v++)
            if (cost[u][v] != 0 && !settled[v] && (dist[v] == NONE || dist[u] + cost[u][v] < dist[v]))
                dist[v] = dist[u] + cost[u][v];
    }
}

/* Bellman-Ford: relax every road N-1 times. Correct with negative edges
   as long as there is no negative cycle. */
static void bellman_ford(long long dist[N])
{
    for (int v = 0; v < N; v++)
        dist[v] = NONE;
    dist[DEPOT] = 0;
    for (int round = 0; round < N - 1; round++)
        for (int u = 0; u < N; u++)
            for (int v = 0; v < N; v++)
                if (cost[u][v] != 0 && dist[u] != NONE && (dist[v] == NONE || dist[u] + cost[u][v] < dist[v]))
                    dist[v] = dist[u] + cost[u][v];
}

int main(void)
{
    long long d[N], b[N];
    textbook_dijkstra(d);
    bellman_ford(b);
    printf("%-8s %10s %13s\n", "Place", "Dijkstra", "Bellman-Ford");
    for (int v = 0; v < N; v++)
        printf("%-8s %7lld Wh %10lld Wh%s\n", names[v], d[v], b[v], d[v] != b[v] ? "   <- wrong" : "");
    return 0;
}
