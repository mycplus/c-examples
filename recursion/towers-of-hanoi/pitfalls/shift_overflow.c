/* shift_overflow.c - DO NOT COPY. 2^n - 1 computed as (1 << n) - 1 in int.
 * For n >= 31 the shift is undefined behaviour in C. */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        int n = atoi(argv[i]);
        int moves = (1 << n) - 1;
        printf("%d disks: %d moves\n", n, moves);
    }
    return 0;
}
