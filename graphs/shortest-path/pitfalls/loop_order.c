/* Pitfall: putting the k loop of Floyd-Warshall on the inside.

   The three loops look interchangeable, but k, the "allowed stop", must be
   the outer loop. Round k may only build on results that already allow
   stops 0..k-1. With k innermost, dist[i][j] is finished before the
   shorter pieces it depends on have been computed. */
#include <stdio.h>

#define N 5
#define NONE 1000000
static const char *const names[N] = {"Ashford", "Bramley", "Corwen", "Dunmore", "Elmstead"};

static void load(int d[N][N])
{
    /* One chain of roads: Ashford - Elmstead - Dunmore - Corwen - Bramley */
    static const int roads[][3] = {{0, 4, 150}, {4, 3, 40}, {3, 2, 51}, {2, 1, 30}};
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            d[i][j] = i == j ? 0 : NONE;
    for (size_t r = 0; r < sizeof roads / sizeof roads[0]; r++)
        d[roads[r][0]][roads[r][1]] = d[roads[r][1]][roads[r][0]] = roads[r][2];
}

static void show(int km)
{
    if (km >= NONE)
        printf(" %9s", "no route");
    else
        printf(" %9d", km);
}

static void relax(int d[N][N], int i, int k, int j)
{
    if (d[i][k] + d[k][j] < d[i][j])
        d[i][j] = d[i][k] + d[k][j];
}

int main(void)
{
    int right[N][N], wrong[N][N];
    load(right);
    load(wrong);
    for (int k = 0; k < N; k++)                         /* k outermost: correct */
        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++)
                relax(right, i, k, j);
    for (int i = 0; i < N; i++)                         /* k innermost: wrong */
        for (int j = 0; j < N; j++)
            for (int k = 0; k < N; k++)
                relax(wrong, i, k, j);

    int bad = 0;
    printf("%-9s %-9s %9s %9s\n", "From", "To", "k outer", "k inner");
    for (int i = 0; i < N; i++)
        for (int j = i + 1; j < N; j++)
            if (right[i][j] != wrong[i][j]) {
                printf("%-9s %-9s", names[i], names[j]);
                show(right[i][j]);
                show(wrong[i][j]);
                putchar('\n');
                bad++;
            }
    printf("%d of %d pairs wrong with k innermost\n", bad, N * (N - 1) / 2);
    return 0;
}
