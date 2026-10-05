/* Tests for src/knapsack_01.c. The source file is included directly so the
   article's listing stays a single, complete program; its main() is renamed
   here so this file can supply its own. */
#define main knapsack_demo_main
#include "../src/knapsack_01.c"
#undef main

static int failures = 0;

static void check(int ok, const char *what, int line)
{
    if (!ok) {
        fprintf(stderr, "FAIL line %d: %s\n", line, what);
        failures++;
    }
}
#define CHECK(cond) check((cond), #cond, __LINE__)

/* Exhaustive search over all 2^n subsets. Only for small n. */
static long long brute_force(const Item *items, size_t n, int capacity)
{
    long long best = 0;
    for (unsigned long mask = 0; mask < (1UL << n); mask++) {
        long long w = 0, v = 0;
        for (size_t i = 0; i < n; i++)
            if (mask & (1UL << i)) {
                w += items[i].weight;
                v += items[i].value;
            }
        if (w <= capacity && v > best)
            best = v;
    }
    return best;
}

/* Simple, seedable generator so every run tests the same instances. */
static unsigned long long rng_state = 88172645463325252ULL;
static unsigned rnd(unsigned bound)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return (unsigned)(rng_state % bound);
}

static void test_article_example(void)
{
    const Item items[] = {
        {"A", 10, 60}, {"B", 20, 100}, {"C", 30, 120}, {"D", 5, 50},
    };
    long long best = -1;
    bool take[4];
    CHECK(knapsack_select(items, 4, 50, &best, take) == 0);
    CHECK(best == 230);
    CHECK(take[0] && !take[1] && take[2] && take[3]);
    CHECK(knapsack_value(items, 4, 50, &best) == 0 && best == 230);
}

static void test_edge_cases(void)
{
    const Item one[] = {{"X", 7, 40}};
    const Item zero_w[] = {{"Z", 0, 9}, {"Y", 3, 5}};
    const Item negative[] = {{"N", -1, 5}};
    const Item huge[] = {{"H1", 1, LLONG_MAX}, {"H2", 1, 1}};
    long long best = -1;
    bool take[2];

    CHECK(knapsack_value(NULL, 0, 10, &best) == 0 && best == 0);   /* no items */
    CHECK(knapsack_value(one, 1, 0, &best) == 0 && best == 0);     /* zero capacity */
    CHECK(knapsack_value(one, 1, 6, &best) == 0 && best == 0);     /* item too heavy */
    CHECK(knapsack_value(one, 1, 7, &best) == 0 && best == 40);    /* exact fit */
    CHECK(knapsack_select(zero_w, 2, 0, &best, take) == 0 && best == 9 && take[0] && !take[1]);
    CHECK(knapsack_select(zero_w, 2, 3, &best, take) == 0 && best == 14 && take[0] && take[1]);
    CHECK(knapsack_value(negative, 1, 10, &best) == -1);           /* invalid weight */
    CHECK(knapsack_value(one, 1, -1, &best) == -1);                /* invalid capacity */
    CHECK(knapsack_value(huge, 2, 2, &best) == -1);                /* total would overflow */
    CHECK(knapsack_value(one, 1, 7, NULL) == -1);
}

static void test_against_brute_force(int rounds)
{
    Item items[14];
    bool take[14];
    for (int r = 0; r < rounds; r++) {
        size_t n = rnd(15);                       /* 0..14 items */
        int capacity = (int)rnd(101);             /* 0..100 */
        for (size_t i = 0; i < n; i++) {
            items[i].name = "r";
            items[i].weight = (int)rnd(41);       /* 0..40, includes zero */
            items[i].value = (long long)rnd(1001);
        }
        long long expect = brute_force(items, n, capacity);
        long long got_value = -1, got_select = -1;
        CHECK(knapsack_value(items, n, capacity, &got_value) == 0);
        CHECK(knapsack_select(items, n, capacity, &got_select, take) == 0);
        CHECK(got_value == expect);
        CHECK(got_select == expect);

        /* The chosen set must actually fit and add up to the reported value. */
        long long w = 0, v = 0;
        for (size_t i = 0; i < n; i++)
            if (take[i]) {
                w += items[i].weight;
                v += items[i].value;
            }
        CHECK(w <= capacity);
        CHECK(v == got_select);
        if (failures > 10)
            return;
    }
}

int main(void)
{
    const int rounds = 20000;
    test_article_example();
    test_edge_cases();
    test_against_brute_force(rounds);
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    printf("All tests passed (%d random instances checked against brute force)\n", rounds);
    return EXIT_SUCCESS;
}
