#include <stdio.h>

#define ROWS 3
#define COLS 4

int main(void)
{
    int grid[ROWS][COLS] = {
        {11, 12, 13, 14},
        {21, 22, 23, 24},
        {31, 32, 33, 34},
    };

    printf("sizeof grid = %zu bytes, one row = %zu bytes\n", sizeof grid, sizeof grid[0]);

    /* Each element's distance from the first one, in bytes. */
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            long offset = (long)((char *)&grid[r][c] - (char *)&grid[0][0]);
            printf("%3ld", offset);
        }
        printf("   <- row %d\n", r);
    }
    printf("grid[2][1] = %d sits at (2 * %d + 1) * %zu = %zu bytes\n", grid[2][1], COLS,
           sizeof grid[0][0], (2 * COLS + 1) * sizeof grid[0][0]);
    return 0;
}
