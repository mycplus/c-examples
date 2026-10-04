/* test_sorting.c - every algorithm against qsort() on 20,000 random arrays
 * of 0 to 64 elements, including INT_MIN, INT_MAX and heavy duplication. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "sorting.h"

static int cmp_int(const void *pa, const void *pb)
{
    int a = *(const int *)pa, b = *(const int *)pb;
    return (a > b) - (a < b);
}

static unsigned long long xs = 0x9E3779B97F4A7C15ULL;
static unsigned long long next(void)
{
    xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17;
    return xs;
}

static void merge_sort_v(int a[], size_t n)
{
    if (merge_sort(a, n) != 0) { fputs("out of memory\n", stderr); exit(2); }
}

int main(void)
{
    static const struct { const char *name; void (*fn)(int *, size_t); } algos[] = {
        { "insertion_sort", insertion_sort }, { "selection_sort", selection_sort },
        { "bubble_sort", bubble_sort },       { "shell_sort", shell_sort },
        { "merge_sort", merge_sort_v },       { "heap_sort", heap_sort },
        { "quick_sort", quick_sort },         { "quick_sort_last", quick_sort_last },
    };
    int in[64], ref[64], out[64];
    int failures = 0;

    for (int t = 0; t < 20000; t++) {
        size_t n = (size_t)(next() % 65);
        int range = (t % 3 == 0) ? 4 : 1000;          /* every third: many duplicates */
        for (size_t i = 0; i < n; i++) {
            unsigned long long r = next();
            in[i] = (r % 17 == 0) ? INT_MIN : (r % 17 == 1) ? INT_MAX
                  : (int)(r % (unsigned)range) - range / 2;
        }
        memcpy(ref, in, n * sizeof in[0]);
        qsort(ref, n, sizeof ref[0], cmp_int);

        for (size_t k = 0; k < sizeof algos / sizeof algos[0]; k++) {
            memcpy(out, in, n * sizeof in[0]);
            algos[k].fn(n ? out : NULL, n);           /* n == 0: pass NULL */
            if (n && memcmp(out, ref, n * sizeof out[0]) != 0) {
                printf("FAIL %s, n = %zu, case %d\n", algos[k].name, n, t);
                failures++;
            }
        }
    }
    printf("%s: 8 algorithms x 20000 arrays of 0-64 elements\n",
           failures ? "FAILED" : "passed");
    return failures != 0;
}
