/* align_speed.c - time align_global, align_local and align_score on random
 * protein pairs of equal length with BLOSUM62 and gaps of 11 + k.
 * Prints the median of 5 runs of each. Not run by ctest. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "align.h"
#include "blosum.h"

static uint64_t state = 99;
static unsigned next(void)
{
    state = state * 6364136223846793005u + 1442695040888963407u;
    return (unsigned)(state >> 33);
}

static double now_ms(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1e3 + (double)ts.tv_nsec / 1e6;
}

static int cmp(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

/* which: 0 global, 1 local, 2 score only */
static double median_ms(const char *a, const char *b, const Scoring *sc, int which, int *score)
{
    double t[5];
    for (int r = 0; r < 5; r++) {
        Alignment al;
        double start = now_ms();
        int err = which == 2 ? align_score(a, b, sc, 0, score)
                : which == 1 ? align_local(a, b, sc, &al) : align_global(a, b, sc, &al);
        t[r] = now_ms() - start;
        if (err != ALIGN_OK) {
            fprintf(stderr, "%s\n", align_error(err));
            exit(1);
        }
        if (which != 2) {
            *score = al.score;
            alignment_free(&al);
        }
    }
    qsort(t, 5, sizeof t[0], cmp);
    return t[2];
}

int main(void)
{
    const Scoring sc = {BLOSUM_LETTERS, BLOSUM62, 11, 1};
    const size_t lengths[] = {500, 1000, 2000, 4000};
    printf("%6s %10s %10s %12s %10s %8s\n", "length", "global ms", "local ms", "score-only ms",
           "tables MB", "ns/cell");
    for (size_t k = 0; k < sizeof lengths / sizeof lengths[0]; k++) {
        size_t n = lengths[k];
        char *a = malloc(n + 1), *b = malloc(n + 1);
        if (a == NULL || b == NULL)
            return 1;
        for (size_t i = 0; i < n; i++) {
            a[i] = BLOSUM_LETTERS[next() % 20];
            b[i] = BLOSUM_LETTERS[next() % 20];
        }
        a[n] = b[n] = '\0';
        int gs, ls, ss;
        double g = median_ms(a, b, &sc, 0, &gs);
        double l = median_ms(a, b, &sc, 1, &ls);
        double s = median_ms(a, b, &sc, 2, &ss);
        double mb = 3.0 * (double)(n + 1) * (double)(n + 1) * sizeof(int) / 1e6;
        printf("%6zu %10.1f %10.1f %12.1f %10.1f %8.2f\n", n, g, l, s, mb,
               s * 1e6 / ((double)n * (double)n));
        if (ss != gs) {
            fprintf(stderr, "score-only %d differs from global %d\n", ss, gs);
            return 1;
        }
        free(a);
        free(b);
    }
    return 0;
}
