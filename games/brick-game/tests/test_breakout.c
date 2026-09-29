/* Tests for the game rules in src/breakout.c and the autopilot. */
#include "autopilot.h"
#include "breakout.h"

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

/* A board with no bricks and the ball placed by hand. */
static void empty_board(Game *g, int x, int y, int dx, int dy)
{
    breakout_init(g, 1);
    memset(g->bricks, 0, sizeof g->bricks);
    g->bricks_left = 1;                  /* keep the game from being won */
    g->bricks[BRICK_ROWS - 1][0] = true; /* ...by one brick far away     */
    g->ball_x = x;
    g->ball_y = y;
    g->dx = dx;
    g->dy = dy;
}

static int count_bricks(const Game *g)
{
    int n = 0;
    for (int r = 0; r < BRICK_ROWS; ++r)
        for (int c = 0; c < BRICK_COLS; ++c)
            n += g->bricks[r][c];
    return n;
}

/* The ball never sits in a solid cell, and the counters match the board. */
static void check_invariants(const Game *g, int lost)
{
    CHECK(g->ball_x >= 0 && g->ball_x < BOARD_W);
    CHECK(g->ball_y >= 0 && g->ball_y < BOARD_H);
    int row = g->ball_y - BRICK_TOP;
    CHECK(!(row >= 0 && row < BRICK_ROWS && g->bricks[row][g->ball_x / BRICK_W]));
    CHECK(!(g->ball_y == BOARD_H - 1 && g->ball_x >= g->paddle_x &&
            g->ball_x < g->paddle_x + PADDLE_W));
    CHECK(count_bricks(g) == g->bricks_left);
    CHECK(g->balls == START_BALLS - lost);
}

static void test_init(void)
{
    Game g;
    breakout_init(&g, 7);
    CHECK(g.bricks_left == 100 && count_bricks(&g) == 100);
    CHECK(g.balls == START_BALLS && g.score == 0 && !g.over);
    CHECK(g.ball_y == BOARD_H - 2 && g.dy == -1);
    CHECK(g.ball_x == g.paddle_x + PADDLE_W / 2);
    CHECK(g.dx == 1 || g.dx == -1);
}

static void test_walls(void)
{
    Game g;
    empty_board(&g, 0, 10, -1, -1);                  /* left wall */
    int ev = breakout_step(&g, INPUT_NONE);
    CHECK((ev & EV_WALL) && g.dx == 1 && g.ball_x == 1 && g.ball_y == 9);

    empty_board(&g, BOARD_W - 1, 10, 1, 1);          /* right wall */
    ev = breakout_step(&g, INPUT_NONE);
    CHECK((ev & EV_WALL) && g.dx == -1 && g.ball_x == BOARD_W - 2);

    empty_board(&g, 30, 0, 1, -1);                   /* ceiling */
    ev = breakout_step(&g, INPUT_NONE);
    CHECK((ev & EV_WALL) && g.dy == 1 && g.ball_y == 1);

    empty_board(&g, 0, 0, -1, -1);                   /* top-left corner */
    breakout_step(&g, INPUT_NONE);
    CHECK(g.dx == 1 && g.dy == 1 && g.ball_x == 1 && g.ball_y == 1);
}

static void test_brick_from_below(void)
{
    Game g;
    empty_board(&g, 4, BRICK_TOP + BRICK_ROWS, 1, -1);
    g.bricks[BRICK_ROWS - 1][1] = true;              /* covers x = 3..5 */
    g.bricks_left = 2;
    int ev = breakout_step(&g, INPUT_NONE);
    CHECK(ev & EV_BRICK);
    CHECK(!g.bricks[BRICK_ROWS - 1][1] && g.bricks_left == 1);
    CHECK(g.score == BRICK_POINTS && g.dy == 1);
}

/* Two bricks touch at a corner: one beside the ball, one above it, and the
 * diagonal cell between them is empty. The ball must not slip through. */
static void test_corner_squeeze(void)
{
    Game g;
    empty_board(&g, 2, BRICK_TOP + 1, 1, -1);
    g.bricks[1][1] = true;       /* beside: x = 3..5, same row as the ball */
    g.bricks[0][0] = true;       /* above:  x = 0..2, row above the ball   */
    g.bricks_left = 3;           /* diagonal cell (3, BRICK_TOP) is empty  */
    int ev = breakout_step(&g, INPUT_NONE);
    CHECK(ev & EV_BRICK);
    CHECK(!g.bricks[1][1] && !g.bricks[0][0]);
    CHECK(g.score == 2 * BRICK_POINTS);
    CHECK(g.dx == -1 && g.dy == 1);
    CHECK(g.ball_x == 1 && g.ball_y == BRICK_TOP + 2);
}

static void test_paddle_aims(void)
{
    Game g;
    for (int offset = 0; offset < PADDLE_W; ++offset) {
        empty_board(&g, 0, BOARD_H - 2, 1, 1);
        g.paddle_x = 20;
        g.ball_x = g.paddle_x + offset;
        if (offset == PADDLE_W - 1)
            g.dx = -1;           /* approach the right end from the right */
        int before = g.dx;
        int ev = breakout_step(&g, INPUT_NONE);
        CHECK(ev & EV_PADDLE);
        CHECK(g.dy == -1);
        if (offset < 2)
            CHECK(g.dx == -1);
        else if (offset > PADDLE_W - 3)
            CHECK(g.dx == 1);
        else
            CHECK(g.dx == before);
    }
}

static void test_paddle_clamps(void)
{
    Game g;
    empty_board(&g, 30, 10, 1, -1);
    for (int i = 0; i < 40; ++i)
        breakout_step(&g, INPUT_LEFT);
    CHECK(g.paddle_x == 0);
    for (int i = 0; i < 40; ++i)
        breakout_step(&g, INPUT_RIGHT);
    CHECK(g.paddle_x == BOARD_W - PADDLE_W);
}

static void test_lost_ball_and_game_over(void)
{
    Game g;
    empty_board(&g, 40, BOARD_H - 1, 1, 1);          /* beside the paddle */
    g.paddle_x = 0;
    g.score = 15;
    int ev = breakout_step(&g, INPUT_NONE);
    CHECK(ev & EV_BALL_LOST);
    CHECK(g.balls == START_BALLS - 1 && g.score == 0);   /* never negative */
    CHECK(g.ball_y == BOARD_H - 2 && g.dy == -1);        /* served again   */

    for (int lost = 1; lost < START_BALLS; ++lost) {
        g.ball_x = 40;
        g.ball_y = BOARD_H - 1;
        g.dy = 1;
        g.paddle_x = 0;
        ev = breakout_step(&g, INPUT_NONE);
    }
    CHECK((ev & EV_GAME_OVER) && g.over && g.balls == 0);
    CHECK(breakout_step(&g, INPUT_NONE) == 0);           /* stays over */
}

static void test_win(void)
{
    Game g;
    empty_board(&g, 1, BRICK_TOP + BRICK_ROWS, 1, -1);
    int ev = breakout_step(&g, INPUT_NONE);      /* the last brick, (0, 4) */
    CHECK((ev & EV_WIN) && g.over && g.bricks_left == 0);
}

/* The autopilot games: the one shown in the article (seed 1) and the one
 * that serves to the other side (seed 2), checked tick by tick. */
static void test_autopilot_games(void)
{
    const struct { uint32_t seed; long ticks; } expect[] = { {1, 3703}, {2, 2128} };
    for (size_t i = 0; i < sizeof expect / sizeof expect[0]; ++i) {
        Game g;
        breakout_init(&g, expect[i].seed);
        long ticks = 0;
        while (!g.over && ticks < 100000) {
            breakout_step(&g, autopilot_input(&g));
            ticks++;
            if (!g.over)
                check_invariants(&g, 0);
        }
        CHECK(g.bricks_left == 0 && g.score == 500 && g.balls == START_BALLS);
        CHECK(ticks == expect[i].ticks);
    }
}

/* Random play: the ball never occupies a solid cell, the brick count and the
 * score agree with the board, and every game ends. */
static void test_random_play_invariants(void)
{
    unsigned long long rng = 88172645463325252ULL;
    for (uint32_t seed = 1; seed <= 300; ++seed) {
        Game g;
        breakout_init(&g, seed);
        int lost = 0;
        long ticks = 0;
        while (!g.over && ticks < 200000) {
            rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
            Input in = (Input)(rng % 3);
            int ev = breakout_step(&g, in);
            lost += (ev & EV_BALL_LOST) != 0;
            ticks++;
            if (g.over)
                break;
            check_invariants(&g, lost);
        }
        CHECK(g.over);
        if (failures > 20)
            return;                                  /* do not flood the log */
    }
}

static void test_render(void)
{
    Game g;
    breakout_init(&g, 1);
    char frame[BREAKOUT_FRAME_SIZE];
    size_t n = breakout_render(&g, frame, sizeof frame);
    CHECK(n == BREAKOUT_FRAME_SIZE - 1 && strlen(frame) == n);
    CHECK(strncmp(frame, "+---", 4) == 0);
    CHECK(strstr(frame, "|[#][#]") != NULL && strchr(frame, 'O') != NULL);
    CHECK(strstr(frame, "=======") != NULL);

    char small[10];                                  /* truncates safely */
    CHECK(breakout_render(&g, small, sizeof small) == n);
    CHECK(strlen(small) == sizeof small - 1);
}

int main(void)
{
    test_init();
    test_walls();
    test_brick_from_below();
    test_corner_squeeze();
    test_paddle_aims();
    test_paddle_clamps();
    test_lost_ball_and_game_over();
    test_win();
    test_autopilot_games();
    test_random_play_invariants();
    test_render();
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    puts("breakout: all tests passed");
    return EXIT_SUCCESS;
}
