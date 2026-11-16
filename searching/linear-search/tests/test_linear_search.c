/* test_linear_search.c - checks every function in linear_search.c against a
 * reference that scans from the other end, on edge cases and on random
 * arrays. Exits with status 1 on any failure. */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "linear_search.h"

static int failures = 0;

#define CHECK(cond, ...)                                              \
    do {                                                              \
        if (!(cond)) {                                                \
            printf("FAILED line %d: ", __LINE__);                     \
            printf(__VA_ARGS__);                                      \
            printf("\n");                                             \
            failures++;                                               \
        }                                                             \
    } while (0)

/* splitmix64: the same numbers on every platform, unlike rand(). */
static uint64_t rng_state = 2026;
static uint64_t next_random(void)
{
    uint64_t z = (rng_state += 0x9e3779b97f4a7c15u);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9u;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebu;
    return z ^ (z >> 31);
}
static int below(int n) { return (int)(next_random() % (uint64_t)n); }

/* The first index of key, found by scanning backwards and keeping the
 * smallest match: a different loop from the one under test. */
static ptrdiff_t reference(const int *a, size_t n, int key)
{
    ptrdiff_t first = -1;
    for (size_t k = n; k-- > 0;)
        if (a[k] == key)
            first = (ptrdiff_t)k;
    return first;
}

static int cmp_int(const void *x, const void *y)
{
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

static bool equals_int(const void *element, const void *ctx)
{
    return *(const int *)element == *(const int *)ctx;
}

static void edge_cases(void)
{
    int one[] = {5}, dup[] = {3, 3, 3};
    CHECK(linear_search(NULL, 0, 5) == -1, "empty array");
    CHECK(sentinel_search(NULL, 0, 5) == -1, "empty array, sentinel");
    CHECK(binary_search(NULL, 0, 5) == -1, "empty array, binary");
    CHECK(linear_search(one, 1, 5) == 0 && linear_search(one, 1, 4) == -1, "one element");
    CHECK(sentinel_search(one, 1, 5) == 0 && sentinel_search(one, 1, 4) == -1 && one[0] == 5,
          "one element, sentinel");
    CHECK(linear_search(dup, 3, 3) == 0 && binary_search(dup, 3, 3) == 0, "first of duplicates");
    int extremes[] = {INT_MIN, 0, INT_MAX};
    CHECK(linear_search(extremes, 3, INT_MAX) == 2 && binary_search(extremes, 3, INT_MIN) == 0,
          "extreme values");
    int mtf[] = {1, 2, 3}, tr[] = {1, 2, 3};
    CHECK(search_move_to_front(mtf, 3, 1) == 0 && mtf[0] == 1 && mtf[1] == 2, "already in front");
    CHECK(search_transpose(tr, 3, 9) == -1 && tr[0] == 1 && tr[2] == 3, "missing key, transpose");
}

static void random_arrays(int trials)
{
    int a[64], copy[64], sorted[64];
    int ok = 0;
    for (int t = 0; t < trials; t++) {
        size_t n = (size_t)below(65);
        int range = 1 + below(40);               /* small ranges give duplicates */
        for (size_t k = 0; k < n; k++)
            a[k] = below(range) - range / 2;
        int key = below(range + 4) - range / 2 - 2;  /* sometimes outside the range */
        ptrdiff_t want = reference(a, n, key);
        int before = failures;

        CHECK(linear_search(a, n, key) == want, "linear_search n=%zu key=%d", n, key);
        memcpy(copy, a, n * sizeof a[0]);
        CHECK(sentinel_search(copy, n, key) == want, "sentinel_search n=%zu key=%d", n, key);
        CHECK(n == 0 || memcmp(copy, a, n * sizeof a[0]) == 0, "sentinel_search changed the array");

        const int *p = search_first(a, n, sizeof a[0], equals_int, &key);
        CHECK(p == (want < 0 ? NULL : a + want), "search_first n=%zu key=%d", n, key);

        memcpy(sorted, a, n * sizeof a[0]);
        qsort(sorted, n, sizeof sorted[0], cmp_int);
        CHECK(binary_search(sorted, n, key) == reference(sorted, n, key),
              "binary_search n=%zu key=%d", n, key);

        /* Move to front: the key comes first, everything else keeps its order. */
        memcpy(copy, a, n * sizeof a[0]);
        CHECK(search_move_to_front(copy, n, key) == want, "move_to_front index");
        if (want >= 0) {
            int expect[64];
            expect[0] = key;
            memcpy(expect + 1, a, (size_t)want * sizeof a[0]);
            memcpy(expect + 1 + want, a + want + 1, (n - (size_t)want - 1) * sizeof a[0]);
            CHECK(memcmp(copy, expect, n * sizeof a[0]) == 0, "move_to_front order n=%zu", n);
        } else {
            CHECK(n == 0 || memcmp(copy, a, n * sizeof a[0]) == 0, "move_to_front changed a miss");
        }

        /* Transpose: the key swaps with its predecessor and nothing else moves. */
        memcpy(copy, a, n * sizeof a[0]);
        CHECK(search_transpose(copy, n, key) == want, "transpose index");
        if (want > 0) {
            int t2 = a[want - 1];
            a[want - 1] = a[want];
            a[want] = t2;
        }
        CHECK(n == 0 || memcmp(copy, a, n * sizeof a[0]) == 0, "transpose order n=%zu", n);
        ok += failures == before;
    }
    printf("random arrays where every function agrees with the reference: %d of %d\n", ok, trials);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);   /* keep output if a sanitizer aborts */
    edge_cases();
    random_arrays(20000);
    printf("%d failed\n", failures);
    return failures != 0;
}
