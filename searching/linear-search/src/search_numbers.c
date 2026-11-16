/* search_numbers.c - read whole numbers from a file (or standard input) and
 * report where each key given on the command line appears.
 *
 *   search_numbers FILE KEY...     or     search_numbers - KEY... < FILE
 *
 * Numbers may be separated by spaces, tabs or new lines. Anything that is
 * not a whole number in the range of int is reported with its line number.
 * Exit status: 0 if every key was found, 1 if any was missing, 2 on error. */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "linear_search.h"

/* Parses s as an int; returns 0 on success. */
static int parse_int(const char *s, int *out)
{
    char *end;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0' || errno == ERANGE || v < INT_MIN || v > INT_MAX)
        return -1;
    *out = (int)v;
    return 0;
}

/* Reads every number in f into a growing array. Returns NULL on error. */
static int *read_numbers(FILE *f, const char *name, size_t *count)
{
    size_t n = 0, cap = 16;
    int *a = malloc(cap * sizeof *a);
    char line[4096];
    for (long lineno = 1; a != NULL && fgets(line, sizeof line, f) != NULL; lineno++) {
        if (strchr(line, '\n') == NULL && !feof(f)) {
            fprintf(stderr, "%s:%ld: line too long\n", name, lineno);
            break;
        }
        for (char *tok = strtok(line, " \t\r\n"); tok != NULL; tok = strtok(NULL, " \t\r\n")) {
            int v;
            if (parse_int(tok, &v) != 0) {
                fprintf(stderr, "%s:%ld: not a whole number: %s\n", name, lineno, tok);
                free(a);
                return NULL;
            }
            if (n == cap) {
                int *bigger = realloc(a, 2 * cap * sizeof *a);
                if (bigger == NULL) {
                    free(a);
                    return NULL;
                }
                a = bigger;
                cap *= 2;
            }
            a[n++] = v;
        }
    }
    if (a != NULL && ferror(f)) {
        fprintf(stderr, "%s: read error\n", name);
        free(a);
        return NULL;
    }
    *count = n;
    return a;
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fputs("usage: search_numbers FILE|- KEY...\n", stderr);
        return 2;
    }
    FILE *f = strcmp(argv[1], "-") == 0 ? stdin : fopen(argv[1], "r");
    if (f == NULL) {
        perror(argv[1]);
        return 2;
    }
    size_t n = 0;
    int *a = read_numbers(f, argv[1], &n);
    if (f != stdin)
        fclose(f);
    if (a == NULL)
        return 2;

    int status = 0;
    for (int k = 2; k < argc; k++) {
        int key;
        if (parse_int(argv[k], &key) != 0) {
            fprintf(stderr, "not a whole number: %s\n", argv[k]);
            status = 2;
            continue;
        }
        ptrdiff_t at = linear_search(a, n, key);
        if (at >= 0) {
            printf("%d: number %td of %zu\n", key, at + 1, n);
        } else {
            printf("%d: not found among %zu numbers\n", key, n);
            if (status == 0)
                status = 1;
        }
    }
    free(a);
    return status;
}
