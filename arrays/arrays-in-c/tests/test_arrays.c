/* Unit tests for the functions in src/functions.c, src/playlist.c and src/seating.c,
   included with main() renamed so the tests use the same code the article shows. */
#define main functions_demo_main
#include "../src/functions.c"
#undef main
#define main playlist_demo_main
#include "../src/playlist.c"
#undef main
#define main seating_demo_main
#include "../src/seating.c"
#undef main

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

static void test_temperatures(void)
{
    double none[1] = {0.0};
    CHECK(average(none, 0) == 0.0);                       /* empty input does not divide by zero */
    double t[4] = {10.0, 20.0, 30.0, 40.0};
    CHECK(average(t, 4) == 25.0);

    int days[4] = {0, 0, 0, 0};
    CHECK(days_above(t, 4, 40.0, days) == 0);             /* "above" is strict */
    CHECK(days_above(t, 4, 15.0, days) == 3);
    CHECK(days[0] == 2 && days[1] == 3 && days[2] == 4);

    double f[3] = {0.0, 100.0, -40.0};
    to_fahrenheit(f, 3);
    CHECK(f[0] == 32.0 && f[1] == 212.0 && f[2] == -40.0);
}

static void test_reverse(void)
{
    const char *none[1] = {"x"};
    reverse(none, 0);
    reverse(none, 1);
    CHECK(strcmp(none[0], "x") == 0);

    for (size_t n = 2; n <= 7; n++) {                     /* even and odd lengths */
        const char *items[7] = {"0", "1", "2", "3", "4", "5", "6"};
        reverse(items, n);
        for (size_t i = 0; i < n; i++)
            CHECK(items[i][0] == (char)('0' + (n - 1 - i)));
    }
}

static void test_seating(void)
{
    char seats[ROWS][COLS];
    memset(seats, '.', sizeof seats);
    CHECK(book(seats, 0, 0));
    CHECK(!book(seats, 0, 0));                            /* already taken */
    CHECK(book(seats, ROWS - 1, COLS - 1));               /* last seat */
    CHECK(!book(seats, ROWS, 0) && !book(seats, 0, COLS));
    CHECK(!book(seats, -1, 0) && !book(seats, 0, -1));
    int taken = 0;
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            taken += seats[r][c] == 'X';
    CHECK(taken == 2);
}

int main(void)
{
    test_temperatures();
    test_reverse();
    test_seating();
    if (failures == 0)
        printf("test_arrays: all checks passed\n");
    return failures != 0;
}
