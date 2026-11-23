/*
 * balloon_shooter.c - a turn-based shooting game on a text grid.
 *
 * Balloons rise from the bottom of an 8 x 8 sky: o rises 1 row a turn,
 * O rises 2. Each turn you shoot one row (or hold fire). The arrow flies
 * while the balloons rise, so aim where a balloon WILL be. 12 balloons,
 * 15 arrows; a balloon that rises above row 1 escapes.
 *
 * Build with GCC or Clang:
 *     gcc -std=c11 -Wall -Wextra -o balloon_shooter balloon_shooter.c
 * Build with Visual Studio (Developer Command Prompt):
 *     cl /W4 balloon_shooter.c
 * Run:
 *     ./balloon_shooter        a new game every second
 *     ./balloon_shooter 42     game number 42: the same game on every computer
 */
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LINE_SIZE      64
#define ROWS           8    /* rows of sky, 1 = top, 8 = bottom */
#define COLS           8    /* columns of sky */
#define TOTAL_BALLOONS 12   /* balloons in one game */
#define ARROWS         15   /* arrows in one game */

/* One balloon. A popped or escaped balloon is only marked inactive: removing
 * it from the middle of the array would mean shifting everything after it. */
typedef struct {
    int  row;      /* 1..ROWS while active */
    int  col;      /* 1..COLS */
    int  speed;    /* rows risen per turn: 1 (slow) or 2 (fast) */
    bool active;   /* false once popped or escaped */
} Balloon;

/* Where the game stands; the main loop runs while it is PLAYING. */
typedef enum { PLAYING, ALL_GONE, OUT_OF_ARROWS } Status;

/* The whole game in one struct, so each function takes a single Game *
 * instead of a long list such as Swat the Fly's int *lives, int *score. */
typedef struct {
    Balloon  balloons[TOTAL_BALLOONS];
    int      spawned;   /* balloons created so far (0..TOTAL_BALLOONS) */
    int      arrows;
    int      popped;
    int      escaped;
    int      turn;      /* starts at 1 */
    uint32_t rng;       /* generator state, starts at the seed */
} Game;

static int    read_line(char *buf, size_t size);
static int    read_int(const char *prompt, int lo, int hi, int *value);
static int    rng_next(uint32_t *state);
static int    rng_below(uint32_t *state, int n);
static int    parse_seed(const char *text, uint32_t *seed);
static void   game_init(Game *g, uint32_t seed);
static int    spawn_balloon(Game *g);
static int    active_count(const Game *g);
static void   draw(const Game *g);
static void   rise(Game *g);
static void   shoot(Game *g, int row);
static void   spawn_wave(Game *g);
static Status game_status(const Game *g);

int main(int argc, char *argv[])
{
    uint32_t seed = (uint32_t)time(NULL);
    Game game;
    int row = 0;

    if (argc > 2 || (argc == 2 && !parse_seed(argv[1], &seed))) {
        fprintf(stderr, "Usage: balloon_shooter [seed]   (seed: a whole number from 0 to 4294967295)\n");
        return 1;
    }

    printf("Balloon Shooter\n===============\n");
    printf("Balloons rise from the bottom of the sky. Pop them before they escape.\n");
    printf("o = slow balloon (rises 1 row)    O = fast balloon (rises 2 rows)\n");
    printf("Your arrow flies while the balloons rise, so aim where a balloon WILL be.\n");
    printf("You have %d arrows for %d balloons.\n", ARROWS, TOTAL_BALLOONS);

    game_init(&game, seed);
    while (game_status(&game) == PLAYING) {     /* the game loop */
        draw(&game);
        if (!read_int("Shoot which row? (1-8, 0 to hold fire): ", 0, ROWS, &row)) {
            printf("\n\nGoodbye!\n");          /* input ended */
            return 0;
        }
        rise(&game);
        if (row > 0)
            shoot(&game, row);
        else
            printf("You hold your fire.\n");
        spawn_wave(&game);
        game.turn++;
    }

    printf("\n");
    if (game.popped == TOTAL_BALLOONS)
        printf("Perfect! You popped all %d balloons.\n", TOTAL_BALLOONS);
    else if (game_status(&game) == ALL_GONE)
        printf("All %d balloons are gone. You popped %d and %d escaped.\n",
               TOTAL_BALLOONS, game.popped, game.escaped);
    else
        printf("Out of arrows! You popped %d of %d balloons.\n", game.popped, TOTAL_BALLOONS);
    return 0;
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

/* Starts a new game: no balloons, ARROWS arrows, turn 1, the generator at
 * seed, and one balloon already in the sky. */
static void game_init(Game *g, uint32_t seed)
{
    memset(g, 0, sizeof *g);
    g->arrows = ARROWS;
    g->turn = 1;
    g->rng = seed;
    spawn_balloon(g);
}

/* Adds a balloon to the bottom row in a random column with a random speed.
 * Only one balloon may use a column. Balloons only move up, so two balloons
 * in different columns can never meet; in one column, a fast balloon would
 * catch a slow one and both would sit in one cell. This rule makes that bug
 * impossible. If the column is taken, it tries the next one to the right,
 * wrapping from COLS back to 1. Returns 1 if a balloon was added, 0 if
 * every column was taken. */
static int spawn_balloon(Game *g)
{
    int col = 1 + rng_below(&g->rng, COLS);
    int speed = 1 + rng_below(&g->rng, 2);      /* always draw both, col first */

    for (int tries = 0; tries < COLS; tries++) {
        bool taken = false;
        for (int i = 0; i < g->spawned; i++)
            if (g->balloons[i].active && g->balloons[i].col == col)
                taken = true;
        if (!taken) {
            Balloon b = { ROWS, 0, 0, true };
            b.col = col;
            b.speed = speed;
            g->balloons[g->spawned++] = b;
            return 1;
        }
        col = col % COLS + 1;                   /* 8 wraps back to 1 */
    }
    return 0;
}

/* Returns how many balloons are in the sky right now. */
static int active_count(const Game *g)
{
    int n = 0;

    for (int i = 0; i < g->spawned; i++)
        if (g->balloons[i].active)
            n++;
    return n;
}

/* Prints a blank line, the status line and the sky: o for a slow balloon,
 * O for a fast one, . for empty sky. */
static void draw(const Game *g)
{
    char sky[ROWS][COLS];

    /* The grid is only a picture, rebuilt every turn; the balloon array is the game. */
    memset(sky, '.', sizeof sky);
    for (int i = 0; i < g->spawned; i++) {
        const Balloon *b = &g->balloons[i];
        if (b->active)
            sky[b->row - 1][b->col - 1] = (b->speed == 2) ? 'O' : 'o';
    }

    printf("\nTurn %d    Arrows: %d    Popped: %d    Escaped: %d\n",
           g->turn, g->arrows, g->popped, g->escaped);
    for (int r = 1; r <= ROWS; r++) {
        printf("%2d |", r);
        for (int c = 1; c <= COLS; c++)
            printf(" %c", sky[r - 1][c - 1]);
        printf("\n");
    }
}

/* Moves every balloon up by its speed. A balloon that rises above row 1
 * escapes: it is marked inactive and counted. */
static void rise(Game *g)
{
    for (int i = 0; i < g->spawned; i++) {
        Balloon *b = &g->balloons[i];
        if (!b->active)
            continue;
        b->row -= b->speed;
        if (b->row < 1) {
            b->active = false;
            g->escaped++;
            printf("A balloon escaped!\n");
        }
    }
}

/* Fires one arrow along row. It pops the balloon in that row with the
 * smallest column, because the arrow comes from the left. */
static void shoot(Game *g, int row)
{
    Balloon *target = NULL;

    g->arrows--;
    for (int i = 0; i < g->spawned; i++) {
        Balloon *b = &g->balloons[i];
        if (b->active && b->row == row && (target == NULL || b->col < target->col))
            target = b;
    }
    if (target == NULL) {
        printf("Your arrow missed row %d.\n", row);
        return;
    }
    target->active = false;
    g->popped++;
    printf("POP! You hit a balloon in row %d.\n", row);
}

/* Adds 0, 1 or 2 new balloons while some are still to come: a roll of
 * 0..3 gives 0 (one time in four), 1 (two in four) or 2 (one in four).
 * The sky is never left empty, and the total never passes TOTAL_BALLOONS. */
static void spawn_wave(Game *g)
{
    if (g->spawned >= TOTAL_BALLOONS)
        return;

    int roll = rng_below(&g->rng, 4);
    int n = (roll == 0) ? 0 : (roll < 3) ? 1 : 2;

    if (n == 0 && active_count(g) == 0)
        n = 1;                                  /* never leave the sky empty */
    if (n > TOTAL_BALLOONS - g->spawned)
        n = TOTAL_BALLOONS - g->spawned;
    for (int k = 0; k < n; k++)
        spawn_balloon(g);
}

/* Returns ALL_GONE when every balloon has been created and none is left in
 * the sky, else OUT_OF_ARROWS when the arrows have run out, else PLAYING.
 * The order matters: a last arrow that pops the last balloon is ALL_GONE. */
static Status game_status(const Game *g)
{
    if (g->spawned == TOTAL_BALLOONS && active_count(g) == 0)
        return ALL_GONE;
    if (g->arrows == 0)
        return OUT_OF_ARROWS;
    return PLAYING;
}
