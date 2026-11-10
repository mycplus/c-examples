/* lcs.c - longest common subsequence by dynamic programming.
 *
 * L[i][j] is the length of the LCS of the first i characters of a and the
 * first j characters of b:
 *     L[i][0] = L[0][j] = 0
 *     L[i][j] = L[i-1][j-1] + 1               if a[i-1] == b[j-1]
 *     L[i][j] = max(L[i-1][j], L[i][j-1])     otherwise
 * The answer is L[n][m]. See lcs.h for the interface. */
#include "lcs.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Table cells are 32-bit: an LCS is never longer than the shorter input. */
typedef uint32_t cell;

size_t lcs_length(const char *a, const char *b)
{
    size_t n = strlen(a), m = strlen(b);
    if (m > n) {                         /* keep the rows as short as possible */
        const char *s = a; a = b; b = s;
        size_t t = n; n = m; m = t;
    }
    /* Two rows of the table are enough: row i needs only row i - 1. */
    size_t *prev = calloc(m + 1, sizeof *prev), *cur = calloc(m + 1, sizeof *cur);
    if (prev == NULL || cur == NULL) {
        free(prev);
        free(cur);
        return SIZE_MAX;
    }
    for (size_t i = 1; i <= n; ++i) {
        for (size_t j = 1; j <= m; ++j) {
            if (a[i - 1] == b[j - 1])
                cur[j] = prev[j - 1] + 1;
            else
                cur[j] = prev[j] > cur[j - 1] ? prev[j] : cur[j - 1];
        }
        size_t *t = prev; prev = cur; cur = t;
    }
    size_t result = prev[m];
    free(prev);
    free(cur);
    return result;
}

/* Allocates the (n+1) x (m+1) table, or returns NULL if its size in bytes
 * would overflow size_t or the lengths do not fit a cell. */
static cell *alloc_table(size_t n, size_t m)
{
    if (n >= UINT32_MAX || m >= UINT32_MAX)
        return NULL;
    size_t rows = n + 1, cols = m + 1;
    if (cols > SIZE_MAX / rows || rows * cols > SIZE_MAX / sizeof(cell))
        return NULL;
    return malloc(rows * cols * sizeof(cell));
}

char *lcs_string(const char *a, const char *b)
{
    size_t n = strlen(a), m = strlen(b);
    cell *L = alloc_table(n, m);
    if (L == NULL)
        return NULL;
#define AT(i, j) L[(i) * (m + 1) + (j)]          /* L[i][j] in the flat array */

    for (size_t i = 0; i <= n; ++i)
        for (size_t j = 0; j <= m; ++j) {
            if (i == 0 || j == 0)
                AT(i, j) = 0;
            else if (a[i - 1] == b[j - 1])
                AT(i, j) = AT(i - 1, j - 1) + 1;
            else
                AT(i, j) = AT(i - 1, j) >= AT(i, j - 1) ? AT(i - 1, j) : AT(i, j - 1);
        }

    size_t len = AT(n, m);
    char *out = malloc(len + 1);       /* allocated even when len is 0 */
    if (out != NULL) {
        out[len] = '\0';
        /* Walk back from L[n][m]: a match belongs to the LCS; otherwise
         * move towards the larger neighbour, up on a tie. */
        size_t i = n, j = m, k = len;
        while (i > 0 && j > 0) {
            if (a[i - 1] == b[j - 1]) {
                out[--k] = a[i - 1];
                --i;
                --j;
            } else if (AT(i - 1, j) >= AT(i, j - 1)) {
                --i;
            } else {
                --j;
            }
        }
    }
#undef AT
    free(L);
    return out;
}

void lcs_print_table(const char *a, const char *b, FILE *out)
{
    size_t n = strlen(a), m = strlen(b);
    cell *L = alloc_table(n, m);
    if (L == NULL)
        return;
#define AT(i, j) L[(i) * (m + 1) + (j)]
    fputs("      ", out);
    for (size_t j = 0; j < m; ++j)
        fprintf(out, " %c", b[j]);
    fputc('\n', out);
    for (size_t i = 0; i <= n; ++i) {
        fprintf(out, "  %c ", i == 0 ? ' ' : a[i - 1]);
        for (size_t j = 0; j <= m; ++j) {
            if (i == 0 || j == 0)
                AT(i, j) = 0;
            else if (a[i - 1] == b[j - 1])
                AT(i, j) = AT(i - 1, j - 1) + 1;
            else
                AT(i, j) = AT(i - 1, j) >= AT(i, j - 1) ? AT(i - 1, j) : AT(i, j - 1);
            fprintf(out, " %u", (unsigned)AT(i, j));
        }
        fputc('\n', out);
    }
#undef AT
    free(L);
}

long lcs_diff(const char *const *a, size_t na, const char *const *b, size_t nb, FILE *out)
{
    cell *L = alloc_table(na, nb);
    if (L == NULL)
        return -1;
#define AT(i, j) L[(i) * (nb + 1) + (j)]
#define SAME(i, j) (strcmp(a[(i)], b[(j)]) == 0)

    /* The same recurrence, with whole lines in place of characters. */
    for (size_t i = 0; i <= na; ++i)
        for (size_t j = 0; j <= nb; ++j) {
            if (i == 0 || j == 0)
                AT(i, j) = 0;
            else if (SAME(i - 1, j - 1))
                AT(i, j) = AT(i - 1, j - 1) + 1;
            else
                AT(i, j) = AT(i - 1, j) >= AT(i, j - 1) ? AT(i - 1, j) : AT(i, j - 1);
        }

    /* Walk back to collect the edit script, then print it forwards. */
    size_t steps = na + nb, k = steps;
    char *op = malloc(steps + 1);
    if (op == NULL) {
        free(L);
        return -1;
    }
    size_t i = na, j = nb;
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && SAME(i - 1, j - 1)) {
            op[--k] = ' ';
            --i;
            --j;
        } else if (j > 0 && (i == 0 || AT(i, j - 1) >= AT(i - 1, j))) {
            op[--k] = '+';                          /* line only in b */
            --j;
        } else {
            op[--k] = '-';                          /* line only in a */
            --i;
        }
    }
    long changes = 0;
    i = j = 0;
    for (; k < steps; ++k) {
        if (op[k] == ' ') {
            fprintf(out, "  %s\n", a[i++]);
            ++j;
        } else if (op[k] == '-') {
            fprintf(out, "- %s\n", a[i++]);
            ++changes;
        } else {
            fprintf(out, "+ %s\n", b[j++]);
            ++changes;
        }
    }
#undef AT
#undef SAME
    free(op);
    free(L);
    return changes;
}
