/* autopilot.c - predicts the ball's landing column and steers under it. */
#include "autopilot.h"

static int landing_column(const Game *g)
{
    int x = g->ball_x, dx = g->dx;
    for (int y = g->ball_y; y < BOARD_H - 2; ++y) {
        if (x + dx < 0 || x + dx >= BOARD_W)
            dx = -dx;                         /* side wall */
        x += dx;
    }
    return x;
}

Input autopilot_input(const Game *g)
{
    if (g->dy < 0)
        return INPUT_NONE;                    /* ball is rising: wait */
    int target = landing_column(g) - PADDLE_W / 2;
    if (target < g->paddle_x)
        return INPUT_LEFT;
    if (target > g->paddle_x)
        return INPUT_RIGHT;
    return INPUT_NONE;
}
