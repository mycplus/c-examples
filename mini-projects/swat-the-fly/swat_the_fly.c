/*
 * swat_the_fly.c - a turn-based memory and arithmetic game.
 *
 * A fly sits on a wall of 9 numbered spots and hops left and right.
 * The program lists its hops in words; you work out where it landed and
 * swat that spot. A hit scores a point, a miss costs one of 3 lives.
 * Each round the fly hops once more. The game lasts 10 rounds.
 *
 * Build with GCC or Clang:
 *     gcc -std=c11 -Wall -Wextra -o swat_the_fly swat_the_fly.c
 * Build with Visual Studio (Developer Command Prompt):
 *     cl /W4 swat_the_fly.c
 * Run:
 *     ./swat_the_fly        a new game every second
 *     ./swat_the_fly 42     game number 42: the same game on every computer
 */
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LINE_SIZE 64
#define SPOTS    9    /* spots on the wall, numbered 1..9 */
#define ROUNDS   10   /* rounds in a full game */
#define LIVES    3    /* misses allowed */
#define MAX_HOPS (2 + ROUNDS)   /* hops in the last round */

static int  read_line(char *buf, size_t size);
static int  read_int(const char *prompt, int lo, int hi, int *value);
static int  rng_next(uint32_t *state);
static int  rng_below(uint32_t *state, int n);
static int  parse_seed(const char *text, uint32_t *seed);
static int  make_path(uint32_t *state, int hops, int path[]);
static void show_wall(int fly);
static void describe_hops(const int path[], int hops);
static void show_path(const int path[], int hops);
static int  play_round(uint32_t *state, int round, int *lives, int *score);

int main(int argc, char *argv[])
{
    uint32_t seed = (uint32_t)time(NULL);
    int round, played = 0, lives = LIVES, score = 0;

    if (argc > 2 || (argc == 2 && !parse_seed(argv[1], &seed))) {
        fprintf(stderr, "Usage: swat_the_fly [seed]   (seed: a whole number from 0 to 4294967295)\n");
        return 1;
    }
    uint32_t state = seed;                      /* the generator starts at the seed */

    printf("Swat the Fly\n============\n");
    printf("Watch the fly hop along the wall, then swat the spot where it lands.\n");
    printf("Each round it hops once more. You have %d lives.\n", LIVES);

    for (round = 1; round <= ROUNDS && lives > 0; round++) {
        if (!play_round(&state, round, &lives, &score)) {
            printf("\n\nGoodbye!\n");          /* input ended */
            return 0;
        }
        played = round;
    }

    printf("\n");
    if (score == ROUNDS)
        printf("Perfect! You swatted all %d flies.\n", ROUNDS);
    else if (lives == 0)
        printf("Game over. You swatted %d of %d flies.\n", score, played);
    else
        printf("Well done. You swatted %d of %d flies.\n", score, ROUNDS);
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

/* Draws a start spot and hops random hops, storing every spot the fly
 * visits in path[0..hops]. A hop that would leave the wall bounces the
 * other way, so the fly always stays on spots 1..SPOTS.
 * Returns the landing spot, path[hops]. */
static int make_path(uint32_t *state, int hops, int path[])
{
    int pos = 1 + rng_below(state, SPOTS);

    path[0] = pos;
    for (int i = 1; i <= hops; i++) {
        int dir = (rng_below(state, 2) == 0) ? -1 : +1;
        if (pos + dir < 1 || pos + dir > SPOTS)
            dir = -dir;                         /* bounce off the end of the wall */
        pos += dir;
        path[i] = pos;
    }
    return pos;
}

/* Prints the spot numbers and, under them, the wall with F where the fly
 * sits and . everywhere else. Each cell is 3 characters wide. */
static void show_wall(int fly)
{
    for (int spot = 1; spot <= SPOTS; spot++)
        printf("%3d", spot);
    printf("\n");
    for (int spot = 1; spot <= SPOTS; spot++)
        printf("%3c", spot == fly ? 'F' : '.');
    printf("\n");
}

/* Prints "The fly buzzes N times: " and each hop as left or right,
 * separated by ", ". A hop is left when the next spot is smaller. */
static void describe_hops(const int path[], int hops)
{
    printf("The fly buzzes %d times: ", hops);
    for (int i = 0; i < hops; i++)
        printf("%s%s", i > 0 ? ", " : "", path[i + 1] < path[i] ? "left" : "right");
    printf("\n");
}

/* Prints the spots the fly visited, for example "Its path: 6 -> 7 -> 6". */
static void show_path(const int path[], int hops)
{
    printf("Its path: %d", path[0]);
    for (int i = 1; i <= hops; i++)
        printf(" -> %d", path[i]);
    printf("\n");
}

/* Plays one round: shows the fly, lists its hops and asks for a swat.
 * A hit adds one to *score; a miss shows the real path and costs a life.
 * Round r has 2 + r hops. Returns 1 to continue, 0 if input ended. */
static int play_round(uint32_t *state, int round, int *lives, int *score)
{
    int path[MAX_HOPS + 1];
    int hops = 2 + round;
    int guess = 0;
    int landing = make_path(state, hops, path);

    printf("\nRound %d of %d    Lives: %d    Score: %d\n\n", round, ROUNDS, *lives, *score);
    show_wall(path[0]);
    printf("\n");
    describe_hops(path, hops);
    if (!read_int("Where did it land? Swat a spot (1-9): ", 1, SPOTS, &guess))
        return 0;
    if (guess == landing) {
        printf("SPLAT! You got it on %d.\n", landing);
        (*score)++;
    } else {
        printf("Missed! The fly landed on %d.\n", landing);
        show_path(path, hops);
        (*lives)--;
    }
    return 1;
}
