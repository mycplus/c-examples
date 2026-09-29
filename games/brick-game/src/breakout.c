/* breakout.c - brick game rules: movement, collisions, scoring, rendering
 * to a text buffer. No terminal or keyboard code lives here.
 */
#include "breakout.h"

#include <stddef.h>
#include <string.h>

typedef enum { CELL_EMPTY, CELL_WALL, CELL_BRICK, CELL_PADDLE, CELL_OUT } Cell;

static uint32_t next_random(uint32_t *state)      /* xorshift32 */
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *state = x;
}

static void serve(Game *g)
{
    g->ball_x = g->paddle_x + PADDLE_W / 2;
    g->ball_y = BOARD_H - 2;                        /* just above the paddle */
    g->dx = (next_random(&g->rng) & 1) ? 1 : -1;
    g->dy = -1;
}

void breakout_init(Game *g, uint32_t seed)
{
    memset(g, 0, sizeof *g);
    for (int r = 0; r < BRICK_ROWS; ++r)
        for (int c = 0; c < BRICK_COLS; ++c)
            g->bricks[r][c] = true;
    g->bricks_left = BRICK_ROWS * BRICK_COLS;
    g->paddle_x = (BOARD_W - PADDLE_W) / 2;
    g->balls = START_BALLS;
    g->rng = seed ? seed : 1;                       /* xorshift needs non-zero */
    serve(g);
}

/* Classifies a cell. Every array index is computed after the bounds checks. */
static Cell cell_at(const Game *g, int x, int y)
{
    if (x < 0 || x >= BOARD_W || y < 0)
        return CELL_WALL;
    if (y >= BOARD_H)
        return CELL_OUT;
    if (y == BOARD_H - 1 && x >= g->paddle_x && x < g->paddle_x + PADDLE_W)
        return CELL_PADDLE;
    int row = y - BRICK_TOP;
    if (row >= 0 && row < BRICK_ROWS && g->bricks[row][x / BRICK_W])
        return CELL_BRICK;
    return CELL_EMPTY;
}

static bool is_solid(Cell c)
{
    return c == CELL_WALL || c == CELL_BRICK || c == CELL_PADDLE;
}

/* Applies a hit on cell (x, y): removes a brick and scores it. */
static void hit(Game *g, Cell c, int x, int y, int *events)
{
    if (c == CELL_BRICK) {
        g->bricks[y - BRICK_TOP][x / BRICK_W] = false;
        g->bricks_left--;
        g->score += BRICK_POINTS;
        *events |= EV_BRICK;
    } else if (c == CELL_PADDLE) {
        *events |= EV_PADDLE;
    } else {
        *events |= EV_WALL;
    }
}

int breakout_step(Game *g, Input in)
{
    int events = 0;
    if (g->over)
        return 0;

    if (in == INPUT_LEFT)
        g->paddle_x -= PADDLE_STEP;
    else if (in == INPUT_RIGHT)
        g->paddle_x += PADDLE_STEP;
    if (g->paddle_x < 0)
        g->paddle_x = 0;
    if (g->paddle_x > BOARD_W - PADDLE_W)
        g->paddle_x = BOARD_W - PADDLE_W;

    int x = g->ball_x, y = g->ball_y;

    /* Check the two cells beside the ball's path before the diagonal one,
     * so the ball cannot slip between two blocks that touch at a corner. */
    Cell side = cell_at(g, x + g->dx, y);
    Cell vert = cell_at(g, x, y + g->dy);
    if (is_solid(side)) {
        hit(g, side, x + g->dx, y, &events);
        g->dx = -g->dx;
    }
    if (is_solid(vert)) {
        if (vert == CELL_PADDLE) {                  /* the paddle's ends aim */
            int offset = x - g->paddle_x;
            if (offset < 2)
                g->dx = -1;
            else if (offset > PADDLE_W - 3)
                g->dx = 1;
        }
        hit(g, vert, x, y + g->dy, &events);
        g->dy = -g->dy;
    }
    if (!is_solid(side) && !is_solid(vert)) {
        Cell diag = cell_at(g, x + g->dx, y + g->dy);
        if (is_solid(diag)) {
            hit(g, diag, x + g->dx, y + g->dy, &events);
            g->dx = -g->dx;
            g->dy = -g->dy;
        }
    }

    /* Move only if the cell ahead is free after bouncing. */
    if (!is_solid(cell_at(g, g->ball_x + g->dx, g->ball_y + g->dy))) {
        g->ball_x += g->dx;
        g->ball_y += g->dy;
    }

    if (g->bricks_left == 0) {
        g->over = true;
        return events | EV_WIN;
    }
    if (g->ball_y >= BOARD_H) {                     /* fell past the paddle */
        events |= EV_BALL_LOST;
        g->score = g->score > LOST_BALL_PENALTY ? g->score - LOST_BALL_PENALTY : 0;
        if (--g->balls == 0) {
            g->over = true;
            return events | EV_GAME_OVER;
        }
        serve(g);
    }
    return events;
}

static void put(char *buf, size_t cap, size_t *n, char ch)
{
    if (*n + 1 < cap)
        buf[*n] = ch;
    (*n)++;
}

/* Writes the playfield as text. Returns the length the frame needs, not
 * counting the NUL; if that is >= cap, the frame was truncated. */
size_t breakout_render(const Game *g, char *buf, size_t cap)
{
    static const char brick_art[BRICK_W + 1] = "[#]";
    size_t n = 0;

    put(buf, cap, &n, '+');
    for (int x = 0; x < BOARD_W; ++x)
        put(buf, cap, &n, '-');
    put(buf, cap, &n, '+');
    put(buf, cap, &n, '\n');
    for (int y = 0; y < BOARD_H; ++y) {
        put(buf, cap, &n, '|');
        for (int x = 0; x < BOARD_W; ++x) {
            char ch = ' ';
            Cell c = cell_at(g, x, y);
            if (x == g->ball_x && y == g->ball_y && !g->over)
                ch = 'O';
            else if (c == CELL_PADDLE)
                ch = '=';
            else if (c == CELL_BRICK)
                ch = brick_art[x % BRICK_W];
            put(buf, cap, &n, ch);
        }
        put(buf, cap, &n, '|');
        put(buf, cap, &n, '\n');
    }
    if (cap > 0)
        buf[n < cap ? n : cap - 1] = '\0';
    return n;
}
