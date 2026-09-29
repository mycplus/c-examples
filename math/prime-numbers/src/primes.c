/* primes.c - test one number for primality, and list primes with the
 * Sieve of Eratosthenes.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic primes.c -o primes
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Trial division by 2, 3 and then 6k - 1 and 6k + 1 up to sqrt(n).
 * i <= n / i is the overflow-safe form of i * i <= n. */
bool is_prime(int64_t n)
{
    if (n < 2)
        return false;
    if (n < 4)
        return true;                        /* 2 and 3 */
    if (n % 2 == 0 || n % 3 == 0)
        return false;
    for (int64_t i = 5; i <= n / i; i += 6)
        if (n % i == 0 || n % (i + 2) == 0)
            return false;
    return true;
}

/* Returns a heap-allocated array of limit flags where flags[k] is true when k is
 * prime, or NULL if memory runs out. The caller frees it. */
bool *sieve(size_t limit)
{
    bool *flags = calloc(limit > 0 ? limit : 1, sizeof *flags);  /* checks n * size */
    if (flags == NULL)
        return NULL;
    for (size_t k = 0; k < limit; ++k)
        flags[k] = k >= 2;
    for (size_t p = 2; limit > 0 && p <= (limit - 1) / p; ++p) {
        if (!flags[p])
            continue;
        for (size_t m = p * p; m < limit; m += p)   /* smaller multiples */
            flags[m] = false;                        /* are already crossed */
    }
    return flags;
}

static size_t count_primes_below(size_t limit)
{
    bool *flags = sieve(limit);
    if (flags == NULL) {
        fputs("out of memory\n", stderr);
        exit(EXIT_FAILURE);
    }
    size_t count = 0;
    for (size_t k = 0; k < limit; ++k)
        count += flags[k];
    free(flags);
    return count;
}

int main(void)
{
    bool *flags = sieve(100);
    if (flags == NULL)
        return EXIT_FAILURE;
    printf("primes below 100:");
    for (size_t k = 0; k < 100; ++k)
        if (flags[k])
            printf(" %zu", k);
    printf("\n");
    free(flags);

    printf("primes below 1000: %zu\n", count_primes_below(1000));
    printf("primes below 1000000: %zu\n", count_primes_below(1000000));

    const int64_t samples[] = { -7, 0, 1, 2, 91, 97 };
    printf("is_prime:");
    for (size_t k = 0; k < sizeof samples / sizeof samples[0]; ++k)
        printf(" %lld %s%s", (long long)samples[k],
               is_prime(samples[k]) ? "true" : "false",
               k + 1 < sizeof samples / sizeof samples[0] ? "," : "\n");
    printf("is_prime(2147483647) = %s\n", is_prime(2147483647) ? "true" : "false");
    printf("is_prime(1000000007) = %s\n", is_prime(1000000007) ? "true" : "false");
    return EXIT_SUCCESS;
}
