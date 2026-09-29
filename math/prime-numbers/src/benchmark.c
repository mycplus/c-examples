/* benchmark.c - count the primes below N by testing each number with
 * is_prime() and with the sieve, and time both. Uses primes.c's functions.
 * Build: gcc -std=c11 -O2 benchmark.c -o benchmark   (primes.c is included)
 */
#define main primes_demo_main
#include "primes.c"
#undef main

#include <time.h>

static double now_seconds(void)
{
    return (double)clock() / CLOCKS_PER_SEC;   /* processor time on POSIX */
}

int main(int argc, char **argv)
{
    size_t limit = argc > 1 ? (size_t)strtoull(argv[1], NULL, 10) : 10000000u;

    double t0 = now_seconds();
    size_t by_trial = 0;
    for (size_t k = 0; k < limit; ++k)
        by_trial += is_prime((int64_t)k);
    double t1 = now_seconds();
    size_t by_sieve = count_primes_below(limit);
    double t2 = now_seconds();

    printf("primes below %zu: %zu by trial division, %zu by the sieve\n",
           limit, by_trial, by_sieve);
    printf("trial division: %8.3f s\n", t1 - t0);
    printf("sieve:          %8.3f s\n", t2 - t1);
    return by_trial == by_sieve ? EXIT_SUCCESS : EXIT_FAILURE;
}
