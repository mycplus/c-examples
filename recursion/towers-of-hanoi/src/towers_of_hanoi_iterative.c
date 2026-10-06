/* towers_of_hanoi_iterative.c - Towers of Hanoi without recursion, in C11.
 * Move k is computed from the bits of k alone.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic towers_of_hanoi_iterative.c -o hanoi_iter
 */
#include <stdint.h>
#include <stdio.h>

/* Prints the 2^n - 1 moves for n disks (1 <= n <= 63) and returns the count. */
static uint64_t hanoi_iterative(unsigned n, char source, char target, char spare)
{
    /* The formula below moves the tower from index 0 to index 2 when n is
     * odd and to index 1 when n is even. */
    const char peg[3] = { source,
                          n % 2 == 1 ? spare : target,
                          n % 2 == 1 ? target : spare };
    const uint64_t last = (UINT64_C(1) << n) - 1;

    for (uint64_t k = 1; k <= last; ++k) {
        unsigned disk = 1;                     /* 1 + trailing zero bits of k */
        for (uint64_t t = k; (t & 1u) == 0; t >>= 1)
            ++disk;
        char from = peg[(k & (k - 1)) % 3];
        char to = peg[((k | (k - 1)) % 3 + 1) % 3];
        printf("Move disk %u from %c to %c\n", disk, from, to);
    }
    return last;
}

int main(void)
{
    const unsigned n = 3;
    uint64_t moves = hanoi_iterative(n, 'A', 'C', 'B');
    printf("%u disks: %llu moves\n", n, (unsigned long long)moves);
    return 0;
}
