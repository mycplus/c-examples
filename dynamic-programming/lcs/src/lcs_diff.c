/* lcs_diff.c - a minimal line diff: prints the lines two files share and
 * the lines that differ, using the LCS of their lines.
 * Usage: lcs_diff OLD NEW   (exit status 0 if the files match, 1 if they
 * differ, 2 on an error, like diff) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lcs.h"

#define MAX_LINE 4096

/* Reads every line of path into a NULL-free array; returns the count, or
 * (size_t)-1 on an error. */
static size_t read_lines(const char *path, char ***out)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror(path);
        return (size_t)-1;
    }
    size_t n = 0, cap = 0;
    char **lines = NULL, buf[MAX_LINE + 2];
    while (fgets(buf, sizeof buf, f) != NULL) {
        size_t len = strcspn(buf, "\r\n");
        if (buf[len] == '\0' && !feof(f)) {
            fprintf(stderr, "%s: line %zu is too long\n", path, n + 1);
            break;
        }
        buf[len] = '\0';
        if (n == cap) {
            cap = cap ? 2 * cap : 64;
            char **grown = realloc(lines, cap * sizeof *grown);
            if (grown == NULL)
                break;
            lines = grown;
        }
        if ((lines[n] = malloc(len + 1)) == NULL)
            break;
        memcpy(lines[n++], buf, len + 1);
    }
    int failed = ferror(f) || !feof(f);
    fclose(f);
    if (failed) {
        for (size_t i = 0; i < n; ++i)
            free(lines[i]);
        free(lines);
        return (size_t)-1;
    }
    *out = lines;
    return n;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fputs("usage: lcs_diff OLD NEW\n", stderr);
        return 2;
    }
    char **a = NULL, **b = NULL;
    size_t na = read_lines(argv[1], &a), nb = (size_t)-1;
    if (na != (size_t)-1)
        nb = read_lines(argv[2], &b);
    long changes = -1;
    if (na != (size_t)-1 && nb != (size_t)-1)
        changes = lcs_diff((const char *const *)a, na, (const char *const *)b, nb, stdout);
    for (size_t i = 0; na != (size_t)-1 && i < na; ++i)
        free(a[i]);
    for (size_t i = 0; nb != (size_t)-1 && i < nb; ++i)
        free(b[i]);
    free(a);
    free(b);
    if (changes < 0)
        return 2;
    return changes == 0 ? 0 : 1;
}
