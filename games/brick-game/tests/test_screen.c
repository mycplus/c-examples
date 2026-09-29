/* Tests for src/screen.c. A small terminal emulator applies the generated
 * output to a character grid, and the grid must then show the new frame. */
#include "autopilot.h"
#include "breakout.h"
#include "screen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;
static void check(bool ok, const char *expr, int line)
{
    if (!ok) {
        fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, line, expr);
        failures++;
    }
}
#define CHECK(cond) check((cond), #cond, __LINE__)

enum { ROWS = 30, COLS = 80 };
static char grid[ROWS][COLS + 1];
static int cur_row, cur_col;

/* Understands exactly what screen_diff emits: ESC[H, ESC[2J, ESC[r;cH,
 * CR, LF and printable characters. Returns false on anything else. */
static bool emulate(const char *s)
{
    while (*s) {
        if (*s == '\x1b') {
            if (strncmp(s, "\x1b[H", 3) == 0) { cur_row = cur_col = 0; s += 3; continue; }
            if (strncmp(s, "\x1b[2J", 4) == 0) {
                for (int r = 0; r < ROWS; ++r) { memset(grid[r], ' ', COLS); grid[r][COLS] = '\0'; }
                s += 4;
                continue;
            }
            int r, c, used;
            if (sscanf(s, "\x1b[%d;%dH%n", &r, &c, &used) == 2) {
                cur_row = r - 1; cur_col = c - 1; s += used;
                continue;
            }
            return false;
        }
        if (*s == '\r') cur_col = 0;
        else if (*s == '\n') cur_row++;
        else if (cur_row < ROWS && cur_col < COLS) grid[cur_row][cur_col++] = *s;
        else return false;
        s++;
    }
    return true;
}

/* Does the grid show frame, line by line? */
static bool shows(const char *frame)
{
    int r = 0;
    for (const char *line = frame; *line; ++r) {
        const char *end = strchr(line, '\n');
        size_t len = end ? (size_t)(end - line) : strlen(line);
        if (r >= ROWS || strncmp(grid[r], line, len) != 0)
            return false;
        line += len + (end != NULL);
    }
    return true;
}

static void test_small_frames(void)
{
    static char out[256];
    size_t n = screen_diff(NULL, "abc\ndef\n", out, sizeof out);
    CHECK(n == strlen(out));
    CHECK(strcmp(out, "\x1b[H\x1b[2J" "abc\r\ndef\r\n") == 0);
    n = screen_diff("abc\ndef\n", "abc\ndxf\n", out, sizeof out);
    CHECK(strcmp(out, "\x1b[2;2Hx") == 0);                 /* one cell */
    n = screen_diff("abc\ndef\n", "abc\ndef\n", out, sizeof out);
    CHECK(n == 0 && out[0] == '\0');                        /* no change */
    n = screen_diff("abcdef\n", "xbcdyz\n", out, sizeof out);
    CHECK(strcmp(out, "\x1b[1;1Hx\x1b[1;5Hyz") == 0);       /* two runs  */
    char tiny[4];
    CHECK(screen_diff(NULL, "abc\n", tiny, sizeof tiny) == 12);   /* truncated */
    CHECK(strlen(tiny) == 3);
}

/* A whole autopilot game: after every tick the emulated screen must match. */
static void test_full_game(void)
{
    static char frames[2][BREAKOUT_FRAME_SIZE], out[4 * BREAKOUT_FRAME_SIZE];
    char *prev = frames[0], *cur = frames[1];
    Game g;
    breakout_init(&g, 2);
    breakout_render(&g, prev, BREAKOUT_FRAME_SIZE);
    screen_diff(NULL, prev, out, sizeof out);
    CHECK(emulate(out) && shows(prev));
    long ticks = 0;
    while (!g.over && ticks < 100000) {
        breakout_step(&g, autopilot_input(&g));
        breakout_render(&g, cur, BREAKOUT_FRAME_SIZE);
        size_t n = screen_diff(prev, cur, out, sizeof out);
        CHECK(n < sizeof out);
        CHECK(emulate(out));
        if (!shows(cur)) {
            CHECK(!"screen differs from frame");
            return;
        }
        char *t = prev; prev = cur; cur = t;
        ticks++;
    }
    CHECK(g.over && ticks > 0);
}

int main(void)
{
    test_small_frames();
    test_full_game();
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    puts("screen: all tests passed");
    return EXIT_SUCCESS;
}
