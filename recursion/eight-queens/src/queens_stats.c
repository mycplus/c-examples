/* queens_stats.c - solutions and search effort for boards from 4 x 4 to 12 x 12.
 * Brute force would test every arrangement of one queen per row and column:
 * n! of them. Backtracking places a queen only on a square that is not
 * attacked, and takes it back when the rows below have no safe square. */
#include <stdio.h>

#include "queens.h"

static bool count_distinct(const queens_board *b, void *ctx)
{
    if (queens_canonical(b))
        ++*(unsigned long long *)ctx;
    return true;
}

int main(void)
{
    printf("   n   solutions   distinct   squares tested   queens placed   arrangements (n!)\n");
    unsigned long long factorial = 1;
    for (int n = 1; n <= 12; n++) {
        factorial *= (unsigned long long)n;
        if (n < 4)
            continue;
        unsigned long long distinct = 0;
        queens_stats s;
        queens_solve(n, count_distinct, &distinct, &s);
        printf("%4d  %10llu  %9llu  %15llu  %14llu  %18llu\n", n, s.solutions, distinct,
               s.squares_tested, s.placements, factorial);
    }
    return 0;
}
