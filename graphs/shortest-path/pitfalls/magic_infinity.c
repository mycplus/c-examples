/* Pitfall: using an ordinary number such as 9999 to mean "no road".

   A freight planner stores lane lengths in kilometres. Every lane is real and
   short enough, but a four-lane journey is longer than the "infinity" value,
   so Floyd-Warshall decides the two ends are not connected at all. */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

#define N 5
static const char *const names[N] = {"Rotterdam", "Warsaw", "Moscow", "Novosibirsk", "Vladivostok"};
/* Approximate road distances in km. */
static const int lanes[][3] = {{0, 1, 1200}, {1, 2, 1260}, {2, 3, 3350}, {3, 4, 5800}};

static void floyd(long long d[N][N], long long none)
{
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            d[i][j] = i == j ? 0 : none;
    for (size_t e = 0; e < sizeof lanes / sizeof lanes[0]; e++)
        d[lanes[e][0]][lanes[e][1]] = d[lanes[e][1]][lanes[e][0]] = lanes[e][2];
    for (int k = 0; k < N; k++)
        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++)
                if (d[i][k] + d[k][j] < d[i][j])        /* no check for "none" */
                    d[i][j] = d[i][k] + d[k][j];
}

int main(void)
{
    long long d[N][N];

    floyd(d, 9999);
    printf("With 9999 meaning \"no road\":\n");
    printf("  %s to %s: %lld%s\n", names[0], names[4], d[0][4], d[0][4] == 9999 ? "  (reported as no route)" : "");

    floyd(d, LLONG_MAX / 4);                            /* far above any real total */
    printf("With a sentinel no real distance can reach:\n");
    printf("  %s to %s: %lld km\n", names[0], names[4], d[0][4]);

    /* The other common choice, INT_MAX, fails differently: INT_MAX + 1200 does
       not fit in an int. Signed overflow is undefined behavior; on ordinary
       two's-complement hardware the sum wraps to a large negative number,
       which then looks like the best route of all. Computed here without
       overflowing, using unsigned arithmetic, which is defined to wrap: */
    uint32_t wrapped = (uint32_t)INT_MAX + 1200u;
    long long as_signed = wrapped > (uint32_t)INT_MAX ? (long long)wrapped - 4294967296LL : (long long)wrapped;
    printf("INT_MAX + 1200 in 32-bit two's complement: %lld\n", as_signed);
    return 0;
}
