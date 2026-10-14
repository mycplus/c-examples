/* queens.c - N-queens by backtracking, one row at a time. */
#include "queens.h"

#include <stdlib.h>

typedef struct {
    queens_board b;
    bool col_used[QUEENS_MAX];
    bool diag_used[2 * QUEENS_MAX - 1];      /* row + col is the same on a / diagonal */
    bool anti_used[2 * QUEENS_MAX - 1];      /* row - col + n - 1 is the same on a \ one */
    queens_visit visit;
    void *ctx;
    queens_stats stats;
    bool stop;
} search;

static void place_row(search *s, int row)
{
    const int n = s->b.n;
    if (row == n) {                           /* every row has a queen */
        s->stats.solutions++;
        if (s->visit != NULL && !s->visit(&s->b, s->ctx))
            s->stop = true;
        return;
    }
    for (int c = 0; c < n && !s->stop; c++) {
        const int d = row + c, a = row - c + n - 1;
        s->stats.squares_tested++;
        if (s->col_used[c] || s->diag_used[d] || s->anti_used[a])
            continue;                         /* attacked: try the next column */
        s->b.col[row] = c;
        s->col_used[c] = s->diag_used[d] = s->anti_used[a] = true;
        s->stats.placements++;
        place_row(s, row + 1);
        s->col_used[c] = s->diag_used[d] = s->anti_used[a] = false;   /* take it back */
    }
}

bool queens_solve(int n, queens_visit visit, void *ctx, queens_stats *stats)
{
    if (n < 1 || n > QUEENS_MAX)
        return false;
    search s = {0};
    s.b.n = n;
    s.visit = visit;
    s.ctx = ctx;
    place_row(&s, 0);
    if (stats != NULL)
        *stats = s.stats;
    return true;
}

bool queens_valid(const queens_board *b)
{
    if (b->n < 1 || b->n > QUEENS_MAX)
        return false;
    for (int r = 0; r < b->n; r++) {
        if (b->col[r] < 0 || b->col[r] >= b->n)
            return false;
        for (int q = 0; q < r; q++)
            if (b->col[q] == b->col[r] || abs(b->col[q] - b->col[r]) == r - q)
                return false;
    }
    return true;
}

/* Square (r, c) after transform t: t & 1 mirrors left-right, t & 2 mirrors
 * top-bottom, t & 4 then swaps rows and columns. Together they give all 8
 * symmetries of the square; swapping first would give one of them twice. */
static void transform(int t, int n, int r, int c, int *out_r, int *out_c)
{
    if (t & 1)
        c = n - 1 - c;
    if (t & 2)
        r = n - 1 - r;
    if (t & 4) {
        const int tmp = r;
        r = c;
        c = tmp;
    }
    *out_r = r;
    *out_c = c;
}

bool queens_canonical(const queens_board *b)
{
    const int n = b->n;
    for (int t = 1; t < 8; t++) {
        int image[QUEENS_MAX];
        for (int r = 0; r < n; r++) {
            int tr, tc;
            transform(t, n, r, b->col[r], &tr, &tc);
            image[tr] = tc;
        }
        for (int r = 0; r < n; r++) {          /* compare row by row */
            if (image[r] < b->col[r])
                return false;                  /* a smaller image exists */
            if (image[r] > b->col[r])
                break;
        }
    }
    return true;
}
