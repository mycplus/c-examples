/* align.c - global and local pairwise alignment with affine gap costs. */
#include "align.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* "Minus infinity" for cells no alignment can reach. It is far enough from
 * INT_MIN that subtracting a gap cost cannot overflow. */
#define NEG (INT_MIN / 4)
#define LIMIT (1 << 20)        /* largest gap cost or table score accepted */

enum { STATE_M, STATE_X, STATE_Y };

typedef struct {
    signed char index[UCHAR_MAX + 1];   /* letter -> row of the table, or -1 */
    size_t size;                        /* number of letters */
    int step;                           /* the most one column can change a score */
} Alphabet;

static int max3(int a, int b, int c)
{
    int m = a > b ? a : b;
    return m > c ? m : c;
}

/* Checks the scoring and builds the letter lookup. Letters are looked up
 * through unsigned char, so bytes above 127 cannot index outside the array. */
static int prepare(const Scoring *sc, Alphabet *ab)
{
    memset(ab->index, -1, sizeof ab->index);
    ab->size = strlen(sc->letters);
    if (ab->size == 0 || ab->size > 64 || sc->gap_open < 0 || sc->gap_extend < 0
        || sc->gap_open > LIMIT || sc->gap_extend > LIMIT)
        return ALIGN_BAD_SCORING;
    ab->step = sc->gap_open + sc->gap_extend;
    for (size_t k = 0; k < ab->size * ab->size; k++) {
        int v = sc->table[k] < 0 ? -sc->table[k] : sc->table[k];
        if (v > LIMIT)
            return ALIGN_BAD_SCORING;
        if (v > ab->step)
            ab->step = v;
    }
    for (size_t k = 0; k < ab->size; k++) {
        unsigned char c = (unsigned char)sc->letters[k];
        ab->index[c] = (signed char)k;
        if (c >= 'A' && c <= 'Z')
            ab->index[c - 'A' + 'a'] = (signed char)k;
    }
    return ALIGN_OK;
}

/* Converts a sequence to table rows. Returns NULL on a bad letter or no memory. */
static unsigned char *encode(const char *s, size_t len, const Alphabet *ab, int *err)
{
    unsigned char *codes = malloc(len + 1);
    if (codes == NULL) {
        *err = ALIGN_NO_MEMORY;
        return NULL;
    }
    for (size_t k = 0; k < len; k++) {
        int code = ab->index[(unsigned char)s[k]];
        if (code < 0) {
            free(codes);
            *err = ALIGN_BAD_LETTER;
            return NULL;
        }
        codes[k] = (unsigned char)code;
    }
    return codes;
}

/* An alignment has at most n + m columns, so no score can exceed
 * (n + m) * step in size. Keeping that below INT_MAX / 8 leaves room for
 * NEG minus a gap cost. */
static int too_long(size_t n, size_t m, const Alphabet *ab)
{
    size_t most = (size_t)(INT_MAX / 8) / (size_t)(ab->step > 0 ? ab->step : 1);
    return n > most || m > most - n;
}

/* Gotoh's three tables. Each cell holds the best score of an alignment of
 * s1[0, i) with s2[0, j) that ends in a particular way:
 *   M  s1[i-1] paired with s2[j-1]
 *   X  s1[i-1] against a gap
 *   Y  s2[j-1] against a gap */
typedef struct {
    size_t n, m, cols;
    int *M, *X, *Y;
} Tables;

#define AT(T, i, j) (T)[(i) * t->cols + (j)]

static int fill(Tables *t, const unsigned char *a, const unsigned char *b,
                const Scoring *sc, size_t width, int local)
{
    size_t n = t->n, m = t->m, cells;
    t->cols = m + 1;
    if (t->cols > SIZE_MAX / (n + 1) || (n + 1) * t->cols > SIZE_MAX / sizeof(int))
        return ALIGN_NO_MEMORY;
    cells = (n + 1) * t->cols;
    t->M = malloc(cells * sizeof(int));
    t->X = malloc(cells * sizeof(int));
    t->Y = malloc(cells * sizeof(int));
    if (t->M == NULL || t->X == NULL || t->Y == NULL)
        return ALIGN_NO_MEMORY;

    const int open = sc->gap_open + sc->gap_extend, ext = sc->gap_extend;
    for (size_t i = 0; i <= n; i++) {
        for (size_t j = 0; j <= m; j++) {
            if (i == 0 || j == 0) {
                /* Global: only the corner is a real start, and the edges are
                 * one gap. Local: an alignment may start anywhere. */
                AT(t->M, i, j) = (local || (i == 0 && j == 0)) ? 0 : NEG;
                AT(t->X, i, j) = (!local && j == 0 && i > 0) ? -(sc->gap_open + (int)i * ext) : NEG;
                AT(t->Y, i, j) = (!local && i == 0 && j > 0) ? -(sc->gap_open + (int)j * ext) : NEG;
                continue;
            }
            int diag = max3(AT(t->M, i - 1, j - 1), AT(t->X, i - 1, j - 1), AT(t->Y, i - 1, j - 1));
            if (local && diag < 0)
                diag = 0;                  /* start a new local alignment here */
            AT(t->M, i, j) = sc->table[a[i - 1] * width + b[j - 1]] + diag;
            AT(t->X, i, j) = max3(AT(t->M, i - 1, j) - open, AT(t->X, i - 1, j) - ext,
                                  AT(t->Y, i - 1, j) - open);
            AT(t->Y, i, j) = max3(AT(t->M, i, j - 1) - open, AT(t->Y, i, j - 1) - ext,
                                  AT(t->X, i, j - 1) - open);
        }
    }
    return ALIGN_OK;
}

/* Walks back from (i, j) in the given state and writes the alignment. */
static int trace(const Tables *t, const char *s1, const char *s2,
                 const unsigned char *a, const unsigned char *b,
                 const Scoring *sc, size_t width, int local,
                 size_t i, size_t j, int state, Alignment *out)
{
    const int open = sc->gap_open + sc->gap_extend, ext = sc->gap_extend;
    size_t cap = i + j, len = 0;
    char *top = malloc(cap + 1), *bottom = malloc(cap + 1);
    if (top == NULL || bottom == NULL) {
        free(top);
        free(bottom);
        return ALIGN_NO_MEMORY;
    }
    out->end1 = i;
    out->end2 = j;
    while (i > 0 || j > 0) {
        if (state == STATE_M) {
            int v = AT(t->M, i, j) - sc->table[a[i - 1] * width + b[j - 1]];
            top[len] = s1[i - 1];
            bottom[len++] = s2[j - 1];
            i--, j--;
            if (local && v == 0)
                break;                     /* the alignment starts at this pair */
            state = v == AT(t->M, i, j) ? STATE_M : v == AT(t->X, i, j) ? STATE_X : STATE_Y;
        } else if (state == STATE_X) {
            int v = AT(t->X, i, j);
            top[len] = s1[i - 1];
            bottom[len++] = '-';
            i--;
            state = v == AT(t->M, i, j) - open ? STATE_M
                  : v == AT(t->X, i, j) - ext  ? STATE_X : STATE_Y;
        } else {
            int v = AT(t->Y, i, j);
            top[len] = '-';
            bottom[len++] = s2[j - 1];
            j--;
            state = v == AT(t->M, i, j) - open ? STATE_M
                  : v == AT(t->Y, i, j) - ext  ? STATE_Y : STATE_X;
        }
    }
    out->start1 = i;
    out->start2 = j;
    for (size_t k = 0; k < len / 2; k++) {    /* the walk ran backwards */
        char c = top[k];
        top[k] = top[len - 1 - k];
        top[len - 1 - k] = c;
        c = bottom[k];
        bottom[k] = bottom[len - 1 - k];
        bottom[len - 1 - k] = c;
    }
    top[len] = bottom[len] = '\0';
    out->top = top;
    out->bottom = bottom;
    return ALIGN_OK;
}

static int align(const char *s1, const char *s2, const Scoring *sc, int local, Alignment *out)
{
    Alphabet ab;
    Tables t = {0};
    unsigned char *a = NULL, *b = NULL;
    int err = prepare(sc, &ab);
    memset(out, 0, sizeof *out);
    if (err != ALIGN_OK)
        return err;
    t.n = strlen(s1);
    t.m = strlen(s2);
    if (too_long(t.n, t.m, &ab))
        return ALIGN_TOO_LONG;
    if ((a = encode(s1, t.n, &ab, &err)) == NULL || (b = encode(s2, t.m, &ab, &err)) == NULL
        || (err = fill(&t, a, b, sc, ab.size, local)) != ALIGN_OK)
        goto done;

    size_t bi = t.n, bj = t.m;
    int state = STATE_M;
    if (local) {
        /* The best local alignment ends at the highest M cell; if no cell is
         * above 0, the best is the empty alignment. */
        out->score = 0;
        bi = bj = 0;
        for (size_t i = 1; i <= t.n; i++)
            for (size_t j = 1; j <= t.m; j++)
                if (t.M[i * t.cols + j] > out->score) {
                    out->score = t.M[i * t.cols + j];
                    bi = i;
                    bj = j;
                }
    } else {
        size_t c = t.n * t.cols + t.m;
        out->score = max3(t.M[c], t.X[c], t.Y[c]);
        state = out->score == t.M[c] ? STATE_M : out->score == t.X[c] ? STATE_X : STATE_Y;
    }
    err = trace(&t, s1, s2, a, b, sc, ab.size, local, bi, bj, state, out);
done:
    free(a);
    free(b);
    free(t.M);
    free(t.X);
    free(t.Y);
    return err;
}

int align_global(const char *s1, const char *s2, const Scoring *sc, Alignment *out)
{
    return align(s1, s2, sc, 0, out);
}

int align_local(const char *s1, const char *s2, const Scoring *sc, Alignment *out)
{
    return align(s1, s2, sc, 1, out);
}

int align_score(const char *s1, const char *s2, const Scoring *sc, int local, int *score)
{
    Alphabet ab;
    size_t n = strlen(s1), m = strlen(s2);
    unsigned char *a = NULL, *b = NULL;
    int *row = NULL, err = prepare(sc, &ab);
    if (err != ALIGN_OK)
        return err;
    if (too_long(n, m, &ab))
        return ALIGN_TOO_LONG;
    if ((a = encode(s1, n, &ab, &err)) == NULL || (b = encode(s2, m, &ab, &err)) == NULL)
        goto done;
    if (m + 1 > SIZE_MAX / sizeof(int) / 6 || (row = malloc(6 * (m + 1) * sizeof(int))) == NULL) {
        err = ALIGN_NO_MEMORY;
        goto done;
    }
    /* Two rows of each table: the previous row (pM, pX, pY) and this one. */
    int *pM = row, *pX = pM + m + 1, *pY = pX + m + 1;
    int *cM = pY + m + 1, *cX = cM + m + 1, *cY = cX + m + 1;
    const int open = sc->gap_open + sc->gap_extend, ext = sc->gap_extend;
    int best = 0;
    for (size_t j = 0; j <= m; j++) {
        pM[j] = (local || j == 0) ? 0 : NEG;
        pX[j] = NEG;
        pY[j] = (!local && j > 0) ? -(sc->gap_open + (int)j * ext) : NEG;
    }
    for (size_t i = 1; i <= n; i++) {
        cM[0] = local ? 0 : NEG;
        cX[0] = local ? NEG : -(sc->gap_open + (int)i * ext);
        cY[0] = NEG;
        for (size_t j = 1; j <= m; j++) {
            int diag = max3(pM[j - 1], pX[j - 1], pY[j - 1]);
            if (local && diag < 0)
                diag = 0;
            cM[j] = sc->table[a[i - 1] * ab.size + b[j - 1]] + diag;
            cX[j] = max3(pM[j] - open, pX[j] - ext, pY[j] - open);
            cY[j] = max3(cM[j - 1] - open, cY[j - 1] - ext, cX[j - 1] - open);
            if (cM[j] > best)
                best = cM[j];
        }
        int *tmp;
        tmp = pM, pM = cM, cM = tmp;
        tmp = pX, pX = cX, cX = tmp;
        tmp = pY, pY = cY, cY = tmp;
    }
    *score = local ? best : max3(pM[m], pX[m], pY[m]);
done:
    free(a);
    free(b);
    free(row);
    return err;
}

void alignment_print(const Alignment *al, const Scoring *sc, FILE *out)
{
    Alphabet ab;
    size_t len = strlen(al->top);
    if (prepare(sc, &ab) != ALIGN_OK)
        return;
    for (size_t from = 0; from < len; from += 60) {
        size_t to = len - from < 60 ? len : from + 60;
        if (from > 0)
            fputc('\n', out);
        fprintf(out, "%.*s\n", (int)(to - from), al->top + from);
        for (size_t k = from; k < to; k++) {
            int x = ab.index[(unsigned char)al->top[k]];
            int y = ab.index[(unsigned char)al->bottom[k]];
            char mark = ' ';
            if (x >= 0 && y >= 0)
                mark = x == y ? '|' : sc->table[(size_t)x * ab.size + (size_t)y] > 0 ? '+' : ' ';
            fputc(mark, out);
        }
        fprintf(out, "\n%.*s\n", (int)(to - from), al->bottom + from);
    }
}

void alignment_free(Alignment *al)
{
    free(al->top);
    free(al->bottom);
    al->top = al->bottom = NULL;
}

const char *align_error(int code)
{
    switch (code) {
    case ALIGN_OK:          return "no error";
    case ALIGN_NO_MEMORY:   return "out of memory";
    case ALIGN_BAD_LETTER:  return "a sequence holds a letter outside the alphabet";
    case ALIGN_TOO_LONG:    return "sequences too long for int scores";
    case ALIGN_BAD_SCORING: return "gap costs or scores out of range";
    default:                return "unknown error";
    }
}
