/* insertion_count.c - comparisons and moves against the inversion count.
 * Five arrangements of 10,000 values. Includes insertion_sort.c directly
 * so SORT_LESS can count; for insertion sort, every comparison that
 * returns true moves exactly one element. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long long comparisons, true_results;
#define SORT_LESS(x, y) (comparisons++, ((x) < (y)) ? (true_results++, 1) : 0)
#include "insertion_sort.c"

#define N 10000

static unsigned long long xs = 88172645463325252ULL;    /* fixed seed */
static unsigned long long next_rand(void)
{
    xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17;
    return xs;
}

static int cmp_int(const void *pa, const void *pb)
{
    int a = *(const int *)pa, b = *(const int *)pb;
    return (a > b) - (a < b);
}

static unsigned long long inversions(const int a[], size_t n)
{
    unsigned long long inv = 0;          /* O(n^2) on purpose: independent */
    for (size_t i = 0; i < n; i++)       /* of the code being measured     */
        for (size_t j = i + 1; j < n; j++)
            if (a[j] < a[i])
                inv++;
    return inv;
}

int main(void)
{
    static const char *names[] = { "sorted", "reversed", "random",
                                   "local (within 8)", "100 far swaps" };
    static int input[N], work[N], ref[N];

    printf("n = %d%18s%13s%13s%13s%13s\n", N, "inversions", "comparisons",
           "moves", "binary cmp", "moves-inv");
    for (int s = 0; s < 5; s++) {
        for (size_t i = 0; i < N; i++)
            input[i] = (int)(next_rand() % 1000000);
        if (s != 2)
            qsort(input, N, sizeof input[0], cmp_int);
        if (s == 1)
            for (size_t i = 0; i < N / 2; i++) {
                int t = input[i]; input[i] = input[N - 1 - i]; input[N - 1 - i] = t;
            }
        if (s == 3)                      /* shuffle inside blocks of 8 */
            for (size_t b = 0; b < N; b += 8)
                for (size_t k = 7; k > 0; k--) {
                    size_t r = (size_t)(next_rand() % (k + 1));
                    int t = input[b + k]; input[b + k] = input[b + r]; input[b + r] = t;
                }
        if (s == 4)                      /* 100 swaps of random positions */
            for (int k = 0; k < 100; k++) {
                size_t i = next_rand() % N, j = next_rand() % N;
                int t = input[i]; input[i] = input[j]; input[j] = t;
            }

        memcpy(ref, input, sizeof ref);
        qsort(ref, N, sizeof ref[0], cmp_int);
        unsigned long long inv = inversions(input, N);

        memcpy(work, input, sizeof work);
        comparisons = true_results = 0;
        insertion_sort(work, N);
        if (memcmp(work, ref, sizeof work) != 0) { puts("WRONG RESULT"); return 1; }
        unsigned long long cmp = comparisons, moves = true_results;

        memcpy(work, input, sizeof work);
        comparisons = 0;
        binary_insertion_sort(work, N);
        if (memcmp(work, ref, sizeof work) != 0) { puts("WRONG RESULT"); return 1; }

        printf("%-22s%13llu%13llu%13llu%13llu%13lld\n", names[s], inv, cmp, moves,
               comparisons, (long long)(moves - inv));
    }
    return 0;
}
