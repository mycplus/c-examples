/* naive_union_find.c - union-find without its two speed-ups. Joining the
 * vertices one after another, always hanging the old set below the new
 * vertex, builds one long chain, and finding a vertex walks down it. The
 * program counts the parent links followed by the finds, with and without
 * union by size and path halving. */
#include <stdio.h>

#define N 20000

static int parent[N], size[N];
static long long steps;
static int smart;

static int find(int x)
{
    while (parent[x] != x) {
        if (smart)
            parent[x] = parent[parent[x]];   /* path halving */
        x = parent[x];
        steps++;
    }
    return x;
}

static void unite(int a, int b)
{
    a = find(a);
    b = find(b);
    if (a == b)
        return;
    if (smart && size[a] < size[b]) {        /* union by size */
        int t = a;
        a = b;
        b = t;
    }
    if (smart) {
        parent[b] = a;                       /* the smaller tree goes below */
        size[a] += size[b];
    } else {
        parent[a] = b;                       /* the old set goes below */
    }
}

static long long run(void)
{
    for (int i = 0; i < N; i++) {
        parent[i] = i;
        size[i] = 1;
    }
    steps = 0;
    for (int i = 1; i < N; i++)
        unite(i - 1, i);
    for (int i = 0; i < N; i++)              /* then find every vertex once */
        find(i);
    return steps;
}

int main(void)
{
    printf("%d vertices joined in a row, then each one found:\n", N);
    smart = 0;
    printf("no balancing, no path compression: %10lld links followed\n", run());
    smart = 1;
    printf("union by size and path halving:    %10lld links followed\n", run());
    return 0;
}
