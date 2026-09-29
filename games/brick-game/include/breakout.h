/* breakout.h - game rules for a terminal brick game. No I/O. */
#ifndef BREAKOUT_H
#define BREAKOUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    BOARD_W     = 60,   /* playfield columns, inside the walls        */
    BOARD_H     = 20,   /* playfield rows; the paddle is on the last   */
    BRICK_W     = 3,    /* columns per brick                           */
    BRICK_COLS  = BOARD_W / BRICK_W,   /* 20 bricks per row            */
    BRICK_ROWS  = 5,
    BRICK_TOP   = 2,    /* first brick row; rows 0-1 are open space    */
    PADDLE_W    = 7,
    PADDLE_STEP = 2,    /* columns moved per key press                 */
    START_BALLS = 3,
    BRICK_POINTS = 5,
    LOST_BALL_PENALTY = 20
};

typedef enum { INPUT_NONE, INPUT_LEFT, INPUT_RIGHT } Input;

/* Bit flags returned by breakout_step() describing what happened. */
enum {
    EV_WALL = 1, EV_PADDLE = 2, EV_BRICK = 4,
    EV_BALL_LOST = 8, EV_WIN = 16, EV_GAME_OVER = 32
};

typedef struct {
    bool     bricks[BRICK_ROWS][BRICK_COLS];
    int      bricks_left;
    int      ball_x, ball_y;   /* cell coordinates inside the walls */
    int      dx, dy;           /* each -1 or +1                     */
    int      paddle_x;         /* leftmost paddle column            */
    int      score;
    int      balls;            /* balls left, including the one in play */
    uint32_t rng;              /* xorshift state for serve direction    */
    bool     over;
} Game;

void breakout_init(Game *g, uint32_t seed);
int  breakout_step(Game *g, Input in);      /* one tick; returns EV_* flags */

/* (BOARD_W + 3) * (BOARD_H + 1) characters plus the NUL. */
enum { BREAKOUT_FRAME_SIZE = (BOARD_W + 3) * (BOARD_H + 1) + 1 };
size_t breakout_render(const Game *g, char *buf, size_t cap);

#endif
