/* merge_count.c - comparisons on 10,000 values, against the worst-case
 * bound, and inversions counted by merge sort against a brute-force count.
 * Includes merge_sort.c directly so SORT_LESS can count. Built twice:
 * with MERGE_SKIP_SORTED=1 (default) and 0. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long long comparisons;
#define SORT_LESS(x, y) (comparisons++, (x) < (y))
#include "merge_sort.c"

#define N 10000

static unsigned long long xs = 88172645463325252ULL;
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

int main(void)
{
    static int input[N], work[N], ref[N];
    static const char *names[] = { "sorted", "reversed", "random", "local (within 8)",
                                   "100 far swaps" };
    /* the same five inputs, generated the same way, as the insertion sort article */
    printf("MERGE_SKIP_SORTED = %d, n = %d\n", MERGE_SKIP_SORTED, N);
    printf("%-18s %11s %11s %13s %13s\n", "input", "top-down", "bottom-up",
           "inversions", "brute force");
    for (int s = 0; s < 5; s++) {
        for (size_t i = 0; i < N; i++)
            input[i] = (int)(next_rand() % 1000000);
        if (s != 2)
            qsort(input, N, sizeof input[0], cmp_int);
        if (s == 1)
            for (size_t i = 0; i < N / 2; i++) {
                int t = input[i]; input[i] = input[N - 1 - i]; input[N - 1 - i] = t;
            }
        if (s == 3)
            for (size_t b = 0; b < N; b += 8)
                for (size_t k = 7; k > 0; k--) {
                    size_t r = (size_t)(next_rand() % (k + 1));
                    int t = input[b + k]; input[b + k] = input[b + r]; input[b + r] = t;
                }
        if (s == 4)
            for (int k = 0; k < 100; k++) {
                size_t i = next_rand() % N, j = next_rand() % N;
                int t = input[i]; input[i] = input[j]; input[j] = t;
            }
        memcpy(ref, input, sizeof ref);
        qsort(ref, N, sizeof ref[0], cmp_int);

        unsigned long long brute = 0;
        for (size_t i = 0; i < N; i++)
            for (size_t j = i + 1; j < N; j++)
                if (input[j] < input[i]) brute++;

        unsigned long long c[2];
        for (int v = 0; v < 2; v++) {
            memcpy(work, input, sizeof work);
            comparisons = 0;
            if ((v == 0 ? merge_sort(work, N) : merge_sort_bottom_up(work, N)) != 0) return 1;
            if (memcmp(work, ref, sizeof work) != 0) { puts("WRONG RESULT"); return 1; }
            c[v] = comparisons;
        }
        memcpy(work, input, sizeof work);
        unsigned long long inv = count_inversions(work, N);
        printf("%-18s %11llu %11llu %13llu %13llu\n", names[s], c[0], c[1], inv, brute);
    }
    return 0;
}
