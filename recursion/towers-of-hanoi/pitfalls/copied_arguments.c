/* copied_arguments.c - DO NOT COPY. The first recursive call passes the pegs
 * in the same order as the outer call, so the n - 1 smaller disks go to the
 * target instead of the spare. The move count is still 2^n - 1; the moves
 * are illegal. A simulator below checks each one. */
#include <stdio.h>

static int peg[3][8], height[3];
static long moves;
static long first_illegal;           /* 0 while every move has been legal */

static void move(int disk, char source, char target)
{
    int f = source - 'A', t = target - 'A';
    ++moves;
    if (first_illegal != 0)
        return;
    if (height[f] == 0 || peg[f][height[f] - 1] != disk ||
        (height[t] > 0 && peg[t][height[t] - 1] < disk)) {
        first_illegal = moves;
        printf("move %ld is illegal: disk %d from %c to %c", moves, disk,
               source, target);
        if (height[t] > 0)
            printf(" lands on disk %d", peg[t][height[t] - 1]);
        printf("\n");
        return;
    }
    peg[t][height[t]++] = peg[f][--height[f]];
}

static void hanoi(int n, char source, char target, char spare)
{
    if (n == 0)
        return;
    hanoi(n - 1, source, target, spare);     /* BUG: should be source, spare, target */
    move(n, source, target);
    hanoi(n - 1, spare, target, source);
}

int main(void)
{
    const int n = 3;
    for (int i = 0; i < n; ++i)
        peg[0][i] = n - i;
    height[0] = n;
    hanoi(n, 'A', 'C', 'B');
    printf("%d disks: %ld moves (2^n - 1 = %d)\n", n, moves, (1 << n) - 1);
    printf("solved: %s\n", first_illegal == 0 && height[2] == n ? "yes" : "no");
    return 0;
}
