/* Tests for src/primes.c, included with main() renamed. */
#define main primes_demo_main
#include "../src/primes.c"
#undef main

static int failures = 0;
static void check(bool ok, const char *expr, int line)
{
    if (!ok) {
        fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, line, expr);
        failures++;
    }
}
#define CHECK(cond) check((cond), #cond, __LINE__)

static void test_small_and_edge_values(void)
{
    const int64_t primes[] = { 2, 3, 5, 7, 11, 13, 97, 7919, 1000003,
                               2147483647, 1000000007, 999999999989 };
    const int64_t composites[] = { INT64_MIN, -7, -1, 0, 1, 4, 6, 9, 25, 49, 91,
                                   121, 561, 1105, 7917,
                                   1000003LL * 1000003LL,       /* prime squared */
                                   1000003LL * 1000033LL,       /* two large primes */
                                   2147483647LL * 2 };
    for (size_t i = 0; i < sizeof primes / sizeof primes[0]; ++i)
        CHECK(is_prime(primes[i]));
    for (size_t i = 0; i < sizeof composites / sizeof composites[0]; ++i)
        CHECK(!is_prime(composites[i]));
}

/* The two methods are independent; they must agree on every number. */
static void test_trial_division_matches_sieve(void)
{
    enum { LIMIT = 200000 };
    bool *flags = sieve(LIMIT);
    CHECK(flags != NULL);
    if (flags == NULL)
        return;
    for (int64_t k = 0; k < LIMIT; ++k)
        if (is_prime(k) != flags[k]) {
            fprintf(stderr, "disagree at %lld\n", (long long)k);
            CHECK(!"trial division and sieve disagree");
            break;
        }
    free(flags);
}

/* Published prime-counting values: pi(10^k). */
static void test_prime_counts(void)
{
    CHECK(count_primes_below(0) == 0);
    CHECK(count_primes_below(1) == 0);
    CHECK(count_primes_below(2) == 0);
    CHECK(count_primes_below(3) == 1);
    CHECK(count_primes_below(10) == 4);
    CHECK(count_primes_below(100) == 25);
    CHECK(count_primes_below(1000) == 168);
    CHECK(count_primes_below(1000000) == 78498);
    CHECK(count_primes_below(10000000) == 664579);
}

int main(void)
{
    test_small_and_edge_values();
    test_trial_division_matches_sieve();
    test_prime_counts();
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    puts("primes: all tests passed");
    return EXIT_SUCCESS;
}
