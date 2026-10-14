/* queens_cli.c - prints the first solution and counts them all.
 * Usage: queens [N]   (N is 1 to 16, default 8) */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "queens.h"

typedef struct {
    queens_board first;
    unsigned long long distinct;
} tally;

static bool record(const queens_board *b, void *ctx)
{
    tally *t = ctx;
    if (t->first.n == 0)
        t->first = *b;
    if (queens_canonical(b))
        t->distinct++;
    return true;                               /* keep searching */
}

int main(int argc, char **argv)
{
    long n = 8;
    if (argc > 2) {
        fprintf(stderr, "usage: queens [N]   (N is 1 to %d)\n", QUEENS_MAX);
        return 2;
    }
    if (argc == 2) {
        char *end;
        errno = 0;
        n = strtol(argv[1], &end, 10);
        if (argv[1][0] < '0' || argv[1][0] > '9' || *end != '\0' || errno != 0 ||
            n < 1 || n > QUEENS_MAX) {
            fprintf(stderr, "usage: queens [N]   (N is 1 to %d)\n", QUEENS_MAX);
            return 2;
        }
    }

    tally t = {0};
    queens_stats s;
    queens_solve((int)n, record, &t, &s);
    if (s.solutions == 0) {
        printf("No way to place %ld queens on a %ld x %ld board.\n", n, n, n);
        return 0;
    }
    printf("First solution, %ld x %ld board:\n", n, n);
    for (int r = 0; r < n; r++) {
        for (int c = 0; c < n; c++)
            printf(" %c", t.first.col[r] == c ? 'Q' : '.');
        printf("\n");
    }
    printf("%llu solution%s, %llu distinct under rotation and reflection.\n",
           s.solutions, s.solutions == 1 ? "" : "s", t.distinct);
    printf("Squares tested: %llu. Queens placed: %llu.\n", s.squares_tested, s.placements);
    return 0;
}
