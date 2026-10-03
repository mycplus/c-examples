/* selection_count.c - comparisons, swaps and shifts on 10,000 values, and
 * the average swap count over 100 random permutations against n - H(n).
 * Includes selection_sort.c directly so the macros can count. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long long comparisons, swaps, shifts;
#define SORT_LESS(x, y) (comparisons++, (x) < (y))
#define SORT_SWAP(x, y) (swaps++, swap_int(x, y))
#define SORT_ON_SHIFT() (shifts++)
#include "selection_sort.c"

#define N 10000

static unsigned long long xs = 88172645463325252ULL;
static unsigned long long next_rand(void)
{
    xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17;
    return xs;
}

static void permutation(int a[], size_t n)           /* Fisher-Yates */
{
    for (size_t i = 0; i < n; i++)
        a[i] = (int)i;
    for (size_t i = n - 1; i > 0; i--) {
        size_t r = (size_t)(next_rand() % (i + 1));
        int t = a[i]; a[i] = a[r]; a[r] = t;
    }
}

static int is_sorted(const int a[], size_t n)
{
    for (size_t i = 1; i < n; i++)
        if (a[i] < a[i - 1]) return 0;
    return 1;
}

int main(void)
{
    static int input[N], work[N];
    static const char *names[] = { "sorted", "reversed", "random", "10 distinct" };
    typedef void (*sort_fn)(int *, size_t);
    static const sort_fn fns[] = { selection_sort, stable_selection_sort, double_selection_sort };
    static const char *fn_names[] = { "selection", "stable", "double" };

    printf("n = %d        %13s %9s %11s %9s %11s\n", N, "comparisons", "swaps",
           "stable cmp", "shifts", "double cmp");
    for (int s = 0; s < 4; s++) {
        permutation(input, N);
        if (s == 0 || s == 1)
            for (size_t i = 0; i < N; i++) input[i] = (s == 0) ? (int)i : (int)(N - 1 - i);
        if (s == 3)
            for (size_t i = 0; i < N; i++) input[i] %= 10;
        unsigned long long c[3], sw = 0, sh = 0;
        for (int f = 0; f < 3; f++) {
            memcpy(work, input, sizeof work);
            comparisons = swaps = shifts = 0;
            fns[f](work, N);
            if (!is_sorted(work, N)) { printf("WRONG RESULT: %s\n", fn_names[f]); return 1; }
            c[f] = comparisons;
            if (f == 0) sw = swaps;
            if (f == 1) sh = shifts;
        }
        printf("%-18s%13llu %9llu %11llu %9llu %11llu\n", names[s], c[0], sw, c[1], sh, c[2]);
    }

    double h = 0;
    for (int k = 1; k <= N; k++) h += 1.0 / k;
    unsigned long long total = 0;
    for (int t = 0; t < 100; t++) {
        permutation(input, N);
        swaps = 0;
        selection_sort(input, N);
        total += swaps;
    }
    printf("\nswaps on 100 random permutations: mean %.2f, predicted n - H(n) = %.2f\n",
           (double)total / 100, N - h);
    return 0;
}
