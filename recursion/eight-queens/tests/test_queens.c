/* Tests for queens.c. */
#include <stdio.h>
#include <stdlib.h>

#include "queens.h"

static int failures = 0;
#define CHECK(cond)                                                              \
    if (!(cond)) {                                                               \
        fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
        failures++;                                                              \
    }

typedef struct {
    unsigned long long distinct, invalid, seen;
    queens_board first;
} tally;

static bool check_each(const queens_board *b, void *ctx)
{
    tally *t = ctx;
    if (t->seen++ == 0)
        t->first = *b;
    if (!queens_valid(b))
        t->invalid++;
    if (queens_canonical(b))
        t->distinct++;
    return true;
}

static bool stop_at_first(const queens_board *b, void *ctx)
{
    (void)b;
    ++*(int *)ctx;
    return false;
}

static void test_counts(void)
{
    /* Solutions and distinct solutions for n = 1..12 (OEIS A000170, A002562). */
    static const unsigned long long all[] = {1, 0, 0, 2, 10, 4, 40, 92, 352, 724, 2680, 14200};
    static const unsigned long long distinct[] = {1, 0, 0, 1, 2, 1, 6, 12, 46, 92, 341, 1787};
    for (int n = 1; n <= 12; n++) {
        tally t = {0};
        queens_stats s;
        CHECK(queens_solve(n, check_each, &t, &s));
        CHECK(s.solutions == all[n - 1] && t.seen == s.solutions);
        CHECK(t.distinct == distinct[n - 1]);
        CHECK(t.invalid == 0);
    }
}

static void test_first_solution_and_effort(void)
{
    tally t = {0};
    queens_stats s;
    queens_solve(8, check_each, &t, &s);
    static const int expected[8] = {0, 4, 7, 5, 2, 6, 1, 3};
    for (int r = 0; r < 8; r++)
        CHECK(t.first.col[r] == expected[r]);
    CHECK(s.placements == 2056);
    CHECK(s.squares_tested == 15720);          /* the count Wirth (1976) gives */
}

static void test_valid(void)
{
    queens_board b = {8, {0, 4, 7, 5, 2, 6, 1, 3}};
    CHECK(queens_valid(&b));
    b.col[7] = 0;                              /* same column as row 0 */
    CHECK(!queens_valid(&b));
    b.col[7] = 3;
    const queens_board diag = {3, {0, 2, 1}};  /* columns differ; rows 1 and 2 share a diagonal */
    CHECK(!queens_valid(&diag));
    b.col[1] = 8;                              /* off the board */
    CHECK(!queens_valid(&b));
    queens_board none = {0, {0}};
    CHECK(!queens_valid(&none));
}

static void test_canonical(void)
{
    /* A solution and its mirror image: exactly one of the two is canonical. */
    const queens_board a = {8, {0, 4, 7, 5, 2, 6, 1, 3}};
    const queens_board m = {8, {7, 3, 0, 2, 5, 1, 6, 4}};
    CHECK(queens_valid(&m));
    CHECK(queens_canonical(&a) != queens_canonical(&m));
}

static void test_stop_and_range(void)
{
    int calls = 0;
    queens_stats s;
    CHECK(queens_solve(8, stop_at_first, &calls, &s));
    CHECK(calls == 1 && s.solutions == 1);
    CHECK(!queens_solve(0, NULL, NULL, &s));
    CHECK(!queens_solve(QUEENS_MAX + 1, NULL, NULL, &s));
    CHECK(queens_solve(1, NULL, NULL, NULL));  /* stats and visit are optional */
}

int main(void)
{
    test_counts();
    test_first_solution_and_effort();
    test_valid();
    test_canonical();
    test_stop_and_range();
    if (failures != 0) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    printf("queens: all tests passed\n");
    return EXIT_SUCCESS;
}
