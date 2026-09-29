/* main.c - the game loop: read keys, advance the game one tick, draw only
 * what changed, and keep the ticks on a fixed schedule.
 * Usage: brick [novice|advanced|expert]
 */
#include "breakout.h"
#include "screen.h"
#include "term.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum { FRAME_CAP = BREAKOUT_FRAME_SIZE + 128, OUT_CAP = 4 * FRAME_CAP };

typedef struct { const char *name; uint32_t tick_ms; } Level;
static const Level levels[] = { {"novice", 70}, {"advanced", 50}, {"expert", 35} };

static void compose(const Game *g, const char *msg, char *frame, size_t cap)
{
    size_t n = breakout_render(g, frame, cap);
    if (n < cap)
        snprintf(frame + n, cap - n, "Score %4d   Balls %d   Bricks %3d   %-26s\n",
                 g->score, g->balls, g->bricks_left, msg);
}

int main(int argc, char **argv)
{
    const Level *level = &levels[0];
    if (argc > 1) {
        level = NULL;
        for (size_t i = 0; i < sizeof levels / sizeof levels[0]; ++i)
            if (strcmp(argv[1], levels[i].name) == 0)
                level = &levels[i];
        if (level == NULL) {
            fprintf(stderr, "usage: %s [novice|advanced|expert]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (!term_init()) {
        fprintf(stderr, "%s: needs an interactive terminal\n", argv[0]);
        return EXIT_FAILURE;
    }
    atexit(term_restore);

    static char frames[2][FRAME_CAP], out[OUT_CAP];
    char *prev = NULL, *cur = frames[0];
    Game g;
    breakout_init(&g, (uint32_t)time(NULL));
    bool paused = false, quit = false;
    const char *msg = "Arrows or A/D, P, Q";
    uint64_t next = term_now_ms();

    while (!quit) {
        Input in = INPUT_NONE;
        for (Key k; !quit && (k = term_key()) != KEY_NONE; ) {  /* drain keys */
            if (k == KEY_QUIT) quit = true;
            else if (k == KEY_PAUSE) paused = !paused;
            else in = (k == KEY_LEFT) ? INPUT_LEFT : INPUT_RIGHT;
        }
        if (!paused && !quit) {
            int ev = breakout_step(&g, in);
            if (ev & EV_WIN)            msg = "You win!";
            else if (ev & EV_GAME_OVER) msg = "Game over";
            else if (ev & EV_BALL_LOST) msg = g.balls == 1 ? "Last ball!" : "Ball lost";
            else if (ev & EV_BRICK)     msg = "";
        }
        compose(&g, paused ? "Paused" : msg, cur, FRAME_CAP);
        size_t len = screen_diff(prev, cur, out, OUT_CAP);
        term_write(out, len < OUT_CAP ? len : OUT_CAP - 1);
        prev = cur;
        cur = (cur == frames[0]) ? frames[1] : frames[0];
        if (g.over)
            break;

        /* Sleep until the next tick's start time, not for a fixed delay,
         * so drawing time does not slow the game down. */
        next += level->tick_ms;
        uint64_t now = term_now_ms();
        if (next > now)
            term_sleep_ms((uint32_t)(next - now));
        else if (now - next > 250)
            next = now;                   /* after a stall, do not race to catch up */
    }
    term_restore();
    printf("Final score: %d\n", g.score);
    return EXIT_SUCCESS;
}
