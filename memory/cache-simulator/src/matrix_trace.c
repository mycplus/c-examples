/* matrix_trace.c - prints the memory trace of summing an N x N matrix of
 * 4-byte ints, row by row or column by column, in the R/W trace format.
 * Usage: matrix_trace row|col [N]   (N defaults to 256, at most 4096) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    const unsigned long long base = 0x10000000ULL;    /* where the matrix "lives" */
    unsigned long n = 256;
    if (argc < 2 || argc > 3 || (strcmp(argv[1], "row") != 0 && strcmp(argv[1], "col") != 0)) {
        fprintf(stderr, "usage: matrix_trace row|col [N]\n");
        return 2;
    }
    if (argc == 3) {
        char *end;
        n = strtoul(argv[2], &end, 10);
        if (*argv[2] < '1' || *argv[2] > '9' || *end != '\0' || n > 4096) {
            fprintf(stderr, "usage: matrix_trace row|col [N]\n");
            return 2;
        }
    }
    const int by_row = strcmp(argv[1], "row") == 0;
    for (unsigned long i = 0; i < n; i++)
        for (unsigned long j = 0; j < n; j++) {
            const unsigned long r = by_row ? i : j, c = by_row ? j : i;
            printf("R 0x%llx\n", base + 4ULL * (r * n + c));   /* sum += m[r][c] */
        }
    return 0;
}
