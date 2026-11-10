/* stem_speed.c - how fast porter_stem runs: stems every word of the test
 * vocabulary 20 times per run, 5 runs, and reports the median time per
 * word. Prints an FNV-1a hash of one pass's stems so different
 * implementations can be checked for identical output.
 * Usage: stem_speed VOC [paper]   (POSIX clock; not run by the tests) */
#define _POSIX_C_SOURCE 199309L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "porter.h"

#define MAX_WORDS 100000
#define WORD_SIZE 64
#define PASSES 20
#define RUNS 5

static char words[MAX_WORDS][WORD_SIZE], buf[WORD_SIZE];

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static int cmp(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

int main(int argc, char **argv)
{
    if (argc < 2)
        return 2;
    enum porter_mode mode = argc > 2 && strcmp(argv[2], "paper") == 0 ? PORTER_PAPER
                                                                     : PORTER_STANDARD;
    FILE *f = fopen(argv[1], "r");
    if (f == NULL)
        return 1;
    size_t n = 0;
    char line[256];
    while (n < MAX_WORDS && fgets(line, sizeof line, f) != NULL) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strlen(line) < WORD_SIZE)
            strcpy(words[n++], line);
    }
    fclose(f);

    uint32_t hash = 2166136261u;            /* FNV-1a over stems + newlines */
    for (size_t i = 0; i < n; ++i) {
        strcpy(buf, words[i]);
        porter_stem(buf, mode);
        for (const char *p = buf;; ++p) {
            hash = (hash ^ (unsigned char)(*p ? *p : '\n')) * 16777619u;
            if (*p == '\0')
                break;
        }
    }

    double t[RUNS];
    volatile size_t sink = 0;              /* keeps the work from being optimized away */
    for (int r = 0; r < RUNS; ++r) {
        double s = now();
        for (int pass = 0; pass < PASSES; ++pass)
            for (size_t i = 0; i < n; ++i) {
                strcpy(buf, words[i]);
                sink += porter_stem(buf, mode);
            }
        t[r] = now() - s;
    }
    qsort(t, RUNS, sizeof t[0], cmp);
    double ns = t[RUNS / 2] / (double)(n * PASSES) * 1e9;
    printf("C (%s mode): %zu words, %.0f ns per word, %.1f million words/s, hash %08x\n",
           mode == PORTER_PAPER ? "paper" : "standard", n, ns, 1e3 / ns, (unsigned)hash);
    return 0;
}
