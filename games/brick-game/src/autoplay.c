/* autoplay.c - plays one game with the autopilot, without a terminal, and
 * reports the result and what drawing each tick would cost.
 * Usage: autoplay [seed]
 */
#include "autopilot.h"
#include "breakout.h"
#include "screen.h"

#include <stdio.h>
#include <stdlib.h>

enum { OUT_CAP = 4 * BREAKOUT_FRAME_SIZE, SNAPSHOT_TICK = 300 };

int main(int argc, char **argv)
{
    uint32_t seed = argc > 1 ? (uint32_t)strtoul(argv[1], NULL, 10) : 1u;
    static char frames[2][BREAKOUT_FRAME_SIZE], out[OUT_CAP];
    char *prev = frames[0], *cur = frames[1];
    Game g;
    breakout_init(&g, seed);
    breakout_render(&g, prev, BREAKOUT_FRAME_SIZE);

    unsigned long long full_bytes = 0, diff_bytes = 0;
    long ticks = 0;
    int paddle_hits = 0, balls_lost = 0;
    while (!g.over && ticks < 100000) {
        int ev = breakout_step(&g, autopilot_input(&g));
        ticks++;
        paddle_hits += (ev & EV_PADDLE) != 0;
        balls_lost += (ev & EV_BALL_LOST) != 0;
        breakout_render(&g, cur, BREAKOUT_FRAME_SIZE);
        full_bytes += screen_diff(NULL, cur, out, OUT_CAP);
        diff_bytes += screen_diff(prev, cur, out, OUT_CAP);
        if (ticks == SNAPSHOT_TICK)
            printf("After %d ticks (score %d):\n%s", SNAPSHOT_TICK, g.score, cur);
        char *t = prev; prev = cur; cur = t;
    }
    printf("\nseed %lu: %s after %ld ticks, score %d, %d paddle hits, %d balls lost\n",
           (unsigned long)seed, g.bricks_left == 0 ? "cleared every brick" : "game over",
           ticks, g.score, paddle_hits, balls_lost);
    printf("output per tick: %.1f bytes redrawing the whole screen, "
           "%.1f bytes redrawing changed cells\n",
           (double)full_bytes / (double)ticks, (double)diff_bytes / (double)ticks);
    return EXIT_SUCCESS;
}
