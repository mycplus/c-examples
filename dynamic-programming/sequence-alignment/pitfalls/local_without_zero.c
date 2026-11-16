/* local_without_zero.c - what Smith-Waterman scores when one of its two
 * local-alignment rules is left out: the zero floor on every cell, and
 * taking the answer from the best cell instead of the bottom-right corner.
 * Linear gaps, +3 for a match, -3 for a mismatch, -2 per gap position. */
#include <stdio.h>
#include <string.h>

#define MAXLEN 32

static int score(const char *a, const char *b, int floor_at_zero, int best_cell)
{
    int H[MAXLEN + 1][MAXLEN + 1];
    size_t n = strlen(a), m = strlen(b);
    int best = -1000;
    for (size_t i = 0; i <= n; i++)
        for (size_t j = 0; j <= m; j++) {
            if (i == 0 || j == 0) {
                /* Without the floor, the edges are runs of gaps, as in a
                 * global alignment. */
                H[i][j] = floor_at_zero ? 0 : -2 * (int)(i + j);
                continue;
            }
            int diag = H[i - 1][j - 1] + (a[i - 1] == b[j - 1] ? 3 : -3);
            int up = H[i - 1][j] - 2, left = H[i][j - 1] - 2;
            int h = diag > up ? diag : up;
            h = h > left ? h : left;
            if (floor_at_zero && h < 0)
                h = 0;
            H[i][j] = h;
            if (h > best)
                best = h;
        }
    return best_cell ? best : H[n][m];
}

int main(void)
{
    const char *a = "TGTTACGG", *b = "GGTTGACTA";
    printf("zero floor, best cell (Smith-Waterman)   %3d\n", score(a, b, 1, 1));
    printf("zero floor, bottom-right corner          %3d\n", score(a, b, 1, 0));
    printf("no floor, best cell                      %3d\n", score(a, b, 0, 1));
    printf("no floor, bottom-right corner (global)   %3d\n", score(a, b, 0, 0));
    return 0;
}
