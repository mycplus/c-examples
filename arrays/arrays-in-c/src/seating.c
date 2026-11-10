/* Real-life example: a cinema seat map stored as a two-dimensional array. */
#include <stdbool.h>
#include <stdio.h>

#define ROWS 4
#define COLS 6

static bool book(char seats[ROWS][COLS], int row, int col)
{
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS || seats[row][col] == 'X')
        return false;                            /* off the map, or already taken */
    seats[row][col] = 'X';
    return true;
}

static void show(char seats[ROWS][COLS])
{
    printf("   ");
    for (int c = 0; c < COLS; c++)
        printf("%d", c + 1);
    printf("\n");
    for (int r = 0; r < ROWS; r++) {
        printf("%c  ", 'A' + r);
        for (int c = 0; c < COLS; c++)
            putchar(seats[r][c]);
        putchar('\n');
    }
}

int main(void)
{
    char seats[ROWS][COLS];
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            seats[r][c] = '.';

    book(seats, 1, 2);                           /* B3 */
    book(seats, 1, 3);                           /* B4 */
    book(seats, 3, 0);                           /* D1 */
    printf("booking B3 again: %s\n", book(seats, 1, 2) ? "ok" : "refused");
    printf("booking E1: %s\n", book(seats, 4, 0) ? "ok" : "refused");

    show(seats);

    int free_seats = 0;
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            free_seats += seats[r][c] == '.';
    printf("%d of %d seats free\n", free_seats, ROWS * COLS);
    return 0;
}
