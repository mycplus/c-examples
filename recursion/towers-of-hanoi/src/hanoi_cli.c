/* hanoi_cli.c - prints the moves for n disks, recursive or iterative.
 * Usage: hanoi_cli N [--iterative]     (0 <= N <= 20)
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hanoi.h"

/* Printing 2^20 - 1 = 1,048,575 lines is already more than anyone reads;
 * the solvers themselves accept up to HANOI_MAX_DISKS. */
#define MAX_PRINTED_DISKS 20L

static void print_move(unsigned disk, char source, char target, void *ctx)
{
    (void)ctx;
    printf("Move disk %u from %c to %c\n", disk, source, target);
}

/* Parses a whole decimal argument in [0, MAX_PRINTED_DISKS]. The first
 * character must be a digit: strtol alone would accept " 3" and "+3". */
static int parse_disks(const char *text, unsigned *out)
{
    char *end;
    if (text[0] < '0' || text[0] > '9')
        return 0;
    errno = 0;
    long v = strtol(text, &end, 10);
    if (end == text || *end != '\0' || errno == ERANGE || v < 0 ||
        v > MAX_PRINTED_DISKS)
        return 0;
    *out = (unsigned)v;
    return 1;
}

int main(int argc, char **argv)
{
    unsigned n;
    int iterative = argc == 3 && strcmp(argv[2], "--iterative") == 0;
    if ((argc != 2 && !iterative) || !parse_disks(argv[1], &n)) {
        fprintf(stderr, "usage: %s N [--iterative]   (N is 0 to %ld)\n",
                argc > 0 ? argv[0] : "hanoi_cli", MAX_PRINTED_DISKS);
        return 2;
    }

    if (iterative)
        hanoi_iterative(n, 'A', 'C', 'B', print_move, NULL);
    else
        hanoi_recursive(n, 'A', 'C', 'B', print_move, NULL);

    uint64_t moves;
    hanoi_move_count(n, &moves);
    printf("%u disks: %llu moves\n", n, (unsigned long long)moves);
    return 0;
}
