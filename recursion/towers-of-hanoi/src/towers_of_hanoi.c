/* towers_of_hanoi.c - the recursive Towers of Hanoi solution in C11.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic towers_of_hanoi.c -o towers_of_hanoi
 */
#include <stdio.h>

/* Moves n disks from peg `source` to peg `target`, using `spare` as the
 * third peg, prints each move, and returns the number of moves made. */
static unsigned long long hanoi(unsigned n, char source, char target, char spare)
{
    if (n == 0)
        return 0;                                    /* nothing to move */

    unsigned long long moves = hanoi(n - 1, source, spare, target);
    printf("Move disk %u from %c to %c\n", n, source, target);
    moves += 1;
    moves += hanoi(n - 1, spare, target, source);
    return moves;
}

int main(void)
{
    const unsigned n = 3;
    unsigned long long moves = hanoi(n, 'A', 'C', 'B');
    printf("%u disks: %llu moves\n", n, moves);
    return 0;
}
