/* naive_recursion.c - the LCS recurrence written as plain recursion, which
 * solves the same subproblems again and again. Counts the calls it makes,
 * for random DNA strings and for strings with no letter in common (the
 * worst case), against the (n + 1) * (n + 1) cells the table fills. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static unsigned long long calls;

static size_t lcs_naive(const char *a, size_t n, const char *b, size_t m)
{
    ++calls;
    if (n == 0 || m == 0)
        return 0;
    if (a[n - 1] == b[m - 1])
        return lcs_naive(a, n - 1, b, m - 1) + 1;
    size_t up = lcs_naive(a, n - 1, b, m), left = lcs_naive(a, n, b, m - 1);
    return up > left ? up : left;
}

static uint64_t rng_state = 5;
static char random_base(void)
{
    rng_state = rng_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return "ACGT"[rng_state >> 62];
}

int main(void)
{
    puts(" length   calls, random DNA   calls, no common letter   table cells");
    for (size_t len = 2; len <= 14; len += 2) {
        char a[32], b[32], x[32], y[32];
        for (size_t i = 0; i < len; ++i) {
            a[i] = random_base();
            b[i] = random_base();
            x[i] = 'A';
            y[i] = 'C';
        }
        calls = 0;
        lcs_naive(a, len, b, len);
        unsigned long long random_calls = calls;
        calls = 0;
        lcs_naive(x, len, y, len);
        printf("%7zu %19llu %25llu %13zu\n", len, random_calls, calls, (len + 1) * (len + 1));
    }
    return 0;
}
