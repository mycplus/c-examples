/* never_stops.c - an iterative backtracking loop with no exit.
 *
 * x[1..8] holds the column of the queen in each row and k is the current
 * row. When every column of row 1 has been tried, k drops to 0. The loop
 * below does not stop there: x[0] acts as one more row, the search starts
 * over, and after the last restart k reaches -1. A guard stops it before it
 * indexes x[-1], which would be undefined behavior. */
#include <stdio.h>
#include <stdlib.h>

static int x[9];

static int safe(int row)
{
    for (int i = 1; i < row; i++)
        if (x[i] == x[row] || abs(x[i] - x[row]) == row - i)
            return 0;
    return 1;
}

int main(void)
{
    const int n = 8;
    int k = 1, solutions = 0;
    while (1) {                       /* BUG: should be while (k >= 1) */
        if (k < 0) {
            printf("Row index reached %d after %d solutions (92 x %d).\n", k, solutions,
                   solutions / 92);
            return 0;
        }
        x[k]++;
        while (x[k] <= n && !safe(k))
            x[k]++;
        if (x[k] <= n) {
            if (k == n)
                solutions++;
            else
                x[++k] = 0;
        } else {
            k--;
        }
    }
}
