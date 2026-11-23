/*
 * sliding_puzzle.c - the sliding-tile puzzle on a 3 x 3 board (the
 * 8-puzzle) or a 4 x 4 board (the 15-puzzle).
 *
 * Put the tiles in order with the gap in the bottom-right corner. Type the
 * number of a tile next to the gap to slide it in; 0 gives up.
 * Easy boards are 20 random slides from the solved board. Hard boards are
 * a fair shuffle of every tile, fixed with one swap if it would be
 * unsolvable: exactly half of all shuffles cannot be solved.
 *
 * Build with GCC or Clang:
 *     gcc -std=c11 -Wall -Wextra -o sliding_puzzle sliding_puzzle.c
 * Build with Visual Studio (Developer Command Prompt):
 *     cl /W4 sliding_puzzle.c
 * Run:
 *     ./sliding_puzzle        a new shuffle every second
 *     ./sliding_puzzle 42     shuffle number 42, the same on every computer
 */
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LINE_SIZE   64
#define MAX_SIZE    4    /* largest board: 4 x 4 */
#define EASY_SLIDES 20   /* random slides for an easy board */

/* The two ways of making a starting board. */
typedef enum { EASY = 1, HARD = 2 } Difficulty;

/* One puzzle: a 2D array of tiles inside a struct, with the gap's place
 * and the player's move count kept next to it. */
typedef struct {
    int size;                          /* 3 or 4 */
    int tile[MAX_SIZE][MAX_SIZE];      /* tile numbers; 0 is the gap */
    int gap_row, gap_col;              /* where the gap is */
    int moves;                         /* moves the player has made */
} Board;

static int  read_line(char *buf, size_t size);
static int  read_int(const char *prompt, int lo, int hi, int *value);
static int  rng_next(uint32_t *state);
static int  rng_below(uint32_t *state, int n);
static int  parse_seed(const char *text, uint32_t *seed);
static void set_solved(Board *b, int size);
static int  is_solved(const Board *b);
static int  count_inversions(const Board *b);
static int  is_solvable(const Board *b);
static int  neighbours(const Board *b, int out[4]);
static int  slide_tile(Board *b, int tile);
static void shuffle_easy(Board *b, uint32_t *rng);
static void shuffle_hard(Board *b, uint32_t *rng);
static void draw_board(const Board *b);

int main(int argc, char *argv[])
{
    uint32_t seed = (uint32_t)time(NULL);
    Board board;
    int size = 0, level = 0, tile = 0;

    if (argc > 2 || (argc == 2 && !parse_seed(argv[1], &seed))) {
        fprintf(stderr, "Usage: sliding_puzzle [seed]   (seed: a whole number from 0 to 4294967295)\n");
        return 1;
    }
    uint32_t rng = seed;                        /* the generator starts at the seed */

    printf("Sliding Puzzle\n==============\n");
    printf("Slide the tiles into order, with the gap in the bottom-right corner.\n");
    printf("Type the number of a tile next to the gap to slide it into the gap.\n");

    int ok = read_int("Board size (3 or 4): ", 3, 4, &size);
    if (ok) {
        printf("1. Easy (20 random slides from the solved board)\n");
        printf("2. Hard (a fair random shuffle)\n");
        ok = read_int("Choose a difficulty (1-2): ", 1, 2, &level);
    }
    if (!ok) {
        printf("\n\nGoodbye!\n");              /* input ended */
        return 0;
    }

    set_solved(&board, size);                   /* no random number is drawn before this */
    switch ((Difficulty)level) {
    case EASY: shuffle_easy(&board, &rng); break;
    case HARD: shuffle_hard(&board, &rng); break;
    }

    for (;;) {
        draw_board(&board);
        for (;;) {                              /* ask until the move is legal */
            if (!read_int("Move which tile? (0 to give up): ", 0, size * size - 1, &tile)) {
                printf("\n\nGoodbye!\n");      /* input ended */
                return 0;
            }
            if (tile == 0) {
                printf("\nYou gave up after %d move%s.\n",
                       board.moves, board.moves == 1 ? "" : "s");
                return 0;
            }
            if (slide_tile(&board, tile))
                break;
            printf("Tile %d is not next to the gap.\n", tile);
        }
        if (is_solved(&board)) {
            draw_board(&board);
            printf("Solved in %d move%s!\n", board.moves, board.moves == 1 ? "" : "s");
            return 0;
        }
    }
}

/* Reads one line into buf and removes the newline. If the line is too long,
 * the rest is thrown away so it cannot become the next answer, and buf is
 * emptied so the caller rejects it instead of using half a number.
 * Returns 1 on success, 0 at end of input. */
static int read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL)
        return 0;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else if (len == size - 1) {               /* buffer full: is there more? */
        int c, extra = 0;
        while ((c = getchar()) != '\n' && c != EOF)
            extra = 1;
        if (extra)
            buf[0] = '\0';
    }
    return 1;
}

/* Prints prompt and reads a whole number from lo to hi into *value.
 * Anything else (letters, an empty line, 12abc, out of range) prints a
 * message and asks again. Returns 1 on success, 0 at end of input. */
static int read_int(const char *prompt, int lo, int hi, int *value)
{
    char buf[LINE_SIZE];
    char *end;

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return 0;
        errno = 0;
        long n = strtol(buf, &end, 10);
        while (*end == ' ' || *end == '\t')
            end++;                              /* allow trailing spaces */
        if (end != buf && *end == '\0' && errno == 0 && n >= lo && n <= hi) {
            *value = (int)n;
            return 1;
        }
        printf("Please enter a whole number from %d to %d.\n", lo, hi);
    }
}

/* The example rand() from the C standard: a linear congruential generator.
 * uint32_t makes the arithmetic wrap modulo 2^32 on every compiler.
 * It returns bits 16..30 because the low bits of this kind of generator
 * are the least random (the lowest bit simply alternates 0, 1, 0, 1...). */
static int rng_next(uint32_t *state)
{
    *state = *state * 1103515245u + 12345u;
    return (int)((*state >> 16) & 0x7FFF);        /* 0 .. 32767 */
}

/* Returns a number from 0 to n - 1. */
static int rng_below(uint32_t *state, int n)
{
    return rng_next(state) % n;
}

/* Reads a seed: a decimal whole number from 0 to 4294967295, with no sign,
 * spaces or other characters. unsigned long is 64 bits on Linux but only
 * 32 on Windows, so the value is checked against UINT32_MAX and ERANGE.
 * Returns 1 on success, 0 otherwise. */
static int parse_seed(const char *text, uint32_t *seed)
{
    char *end;
    unsigned long value;

    if (!isdigit((unsigned char)text[0]))
        return 0;                               /* strtoul would accept "-1" or " 7" */
    errno = 0;
    value = strtoul(text, &end, 10);
    if (*end != '\0' || errno == ERANGE || value > UINT32_MAX)
        return 0;
    *seed = (uint32_t)value;
    return 1;
}

/* Sets up a solved board of the given size: 1, 2, 3... in reading order,
 * with the gap (0) in the bottom-right corner and no moves made. */
static void set_solved(Board *b, int size)
{
    b->size = size;
    for (int i = 0; i < size * size; i++)       /* index i is row i / size, column i % size */
        b->tile[i / size][i % size] = i + 1;
    b->tile[size - 1][size - 1] = 0;
    b->gap_row = size - 1;
    b->gap_col = size - 1;
    b->moves = 0;
}

/* Returns 1 if the tiles are in order with the gap in the bottom-right corner. */
static int is_solved(const Board *b)
{
    int n = b->size * b->size;

    for (int i = 0; i < n - 1; i++)
        if (b->tile[i / b->size][i % b->size] != i + 1)
            return 0;
    return 1;                                   /* the last cell must then be the gap */
}

/* Counts the inversions: pairs of tiles (not the gap) where a larger number
 * comes before a smaller one, reading the board row by row as one list. */
static int count_inversions(const Board *b)
{
    int n = b->size * b->size, count = 0;

    for (int i = 0; i < n; i++) {
        int first = b->tile[i / b->size][i % b->size];
        for (int j = i + 1; j < n; j++) {
            int second = b->tile[j / b->size][j % b->size];
            if (first != 0 && second != 0 && first > second)
                count++;
        }
    }
    return count;
}

/* Returns 1 if the board can be solved. Exactly half of all arrangements can.
 * Odd width (3 x 3): solvable when the number of inversions is even.
 * Even width (4 x 4): solvable when inversions + the gap's row counted from
 * the bottom (starting at 1) is odd.
 * Swapping two tiles changes the inversion count by an odd number, so it
 * always moves a board from one half to the other. */
static int is_solvable(const Board *b)
{
    int inversions = count_inversions(b);

    if (b->size % 2 == 1)
        return inversions % 2 == 0;
    int row_from_bottom = b->size - b->gap_row;
    return (inversions + row_from_bottom) % 2 == 1;
}

/* Fills out with the tiles next to the gap, in the order above, below, left,
 * right, skipping any that are off the board. Returns how many there are. */
static int neighbours(const Board *b, int out[4])
{
    int r = b->gap_row, c = b->gap_col, n = 0;

    if (r > 0)           out[n++] = b->tile[r - 1][c];   /* above */
    if (r < b->size - 1) out[n++] = b->tile[r + 1][c];   /* below */
    if (c > 0)           out[n++] = b->tile[r][c - 1];   /* left */
    if (c < b->size - 1) out[n++] = b->tile[r][c + 1];   /* right */
    return n;
}

/* Slides tile into the gap if it is directly above, below, left or right of
 * the gap (never diagonally), and counts the move. Returns 1 if it moved,
 * 0 if the tile is not next to the gap (nothing changes). */
static int slide_tile(Board *b, int tile)
{
    int r = b->gap_row, c = b->gap_col;
    int dr[4] = { -1, 1, 0, 0 }, dc[4] = { 0, 0, -1, 1 };

    for (int k = 0; k < 4; k++) {
        int tr = r + dr[k], tc = c + dc[k];
        if (tr < 0 || tr >= b->size || tc < 0 || tc >= b->size || b->tile[tr][tc] != tile)
            continue;
        b->tile[r][c] = tile;                   /* the tile moves into the gap... */
        b->tile[tr][tc] = 0;                    /* ...and the gap takes its place */
        b->gap_row = tr;
        b->gap_col = tc;
        b->moves++;
        return 1;
    }
    return 0;
}

/* Makes an easy board: EASY_SLIDES random legal slides from the solved
 * board (more if it happens to end up solved). Only legal moves are used,
 * so the board is always solvable. A tile never slides straight back. */
static void shuffle_easy(Board *b, uint32_t *rng)
{
    int last = 0, done = 0;

    while (done < EASY_SLIDES || is_solved(b)) {
        int near[4], choices[4], count = 0;
        int n = neighbours(b, near);
        for (int k = 0; k < n; k++)
            if (near[k] != last)
                choices[count++] = near[k];
        last = choices[rng_below(rng, count)];
        slide_tile(b, last);
        done++;
    }
    b->moves = 0;                               /* shuffling is not the player's moves */
}

/* Makes a hard board: a fair shuffle of every tile and the gap. If the
 * result is unsolvable, swapping the first two tiles (not the gap) fixes it.
 * A solved board is shuffled again. */
static void shuffle_hard(Board *b, uint32_t *rng)
{
    int n = b->size * b->size;
    int list[MAX_SIZE * MAX_SIZE];

    do {
        for (int i = 0; i < n; i++)
            list[i] = i;                        /* 0 is the gap */
        /* Fisher-Yates: each position gets a random pick from the values not
         * placed yet, so every arrangement is equally likely. */
        for (int i = n - 1; i >= 1; i--) {
            int j = rng_below(rng, i + 1);
            int t = list[i]; list[i] = list[j]; list[j] = t;
        }
        for (int i = 0; i < n; i++) {           /* copy the list into the board */
            b->tile[i / b->size][i % b->size] = list[i];
            if (list[i] == 0) {
                b->gap_row = i / b->size;
                b->gap_col = i % b->size;
            }
        }
        if (!is_solvable(b)) {
            int first = (list[0] != 0) ? 0 : 1;          /* first two tiles, skipping the gap */
            int second = (list[first + 1] != 0) ? first + 1 : first + 2;
            int t = b->tile[first / b->size][first % b->size];
            b->tile[first / b->size][first % b->size] = b->tile[second / b->size][second % b->size];
            b->tile[second / b->size][second % b->size] = t;
        }
    } while (is_solved(b));
    b->moves = 0;
}

/* Prints a blank line, the move count and the board, 3 characters per
 * cell, with . for the gap. */
static void draw_board(const Board *b)
{
    printf("\nMoves: %d\n", b->moves);
    for (int r = 0; r < b->size; r++) {
        for (int c = 0; c < b->size; c++) {
            if (b->tile[r][c] == 0)
                printf("  .");
            else
                printf("%3d", b->tile[r][c]);
        }
        printf("\n");
    }
}
