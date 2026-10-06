/* base_case_one.c - DO NOT COPY. The base case is n == 1, so n == 0 never
 * reaches it: the recursion counts down through -1, -2, ... until the
 * stack runs out. */
#include <stdio.h>
#include <stdlib.h>

static long long moves;

static void hanoi(int n, char source, char target, char spare)
{
    if (n == 1) {
        ++moves;                      /* move the one remaining disk */
        return;
    }
    hanoi(n - 1, source, spare, target);
    ++moves;
    hanoi(n - 1, spare, target, source);
}

int main(int argc, char **argv)
{
    int n = argc > 1 ? atoi(argv[1]) : 0;
    printf("solving for %d disks\n", n);
    fflush(stdout);
    hanoi(n, 'A', 'C', 'B');
    printf("%d disks: %lld moves\n", n, moves);
    return 0;
}
