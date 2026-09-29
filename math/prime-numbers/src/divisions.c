/* divisions.c - how many remainder operations each trial-division bound
 * needs to prove that 2147483647 (2^31 - 1) is prime.
 */
#include <stdint.h>
#include <stdio.h>

static const int64_t N = 2147483647;

int main(void)
{
    uint64_t half = 0, root = 0, odd = 0, six = 0;

    for (int64_t i = 2; i <= N / 2; ++i, ++half)        /* up to n / 2 */
        if (N % i == 0) break;
    for (int64_t i = 2; i <= N / i; ++i, ++root)        /* up to sqrt(n) */
        if (N % i == 0) break;
    odd = 1;                                            /* n % 2 */
    for (int64_t i = 3; i <= N / i; i += 2, ++odd)      /* odd divisors only */
        if (N % i == 0) break;
    six = 2;                                            /* n % 2, n % 3 */
    for (int64_t i = 5; i <= N / i; i += 6, six += 2)   /* 6k - 1, 6k + 1 */
        if (N % i == 0 || N % (i + 2) == 0) break;

    printf("%-26s %13s\n", "divisors tried", "remainders");
    printf("%-26s %13llu\n", "2 .. n/2", (unsigned long long)half);
    printf("%-26s %13llu\n", "2 .. sqrt(n)", (unsigned long long)root);
    printf("%-26s %13llu\n", "2, then odd .. sqrt(n)", (unsigned long long)odd);
    printf("%-26s %13llu\n", "2, 3, then 6k +/- 1", (unsigned long long)six);
    return 0;
}
