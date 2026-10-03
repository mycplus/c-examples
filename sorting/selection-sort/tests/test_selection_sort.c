/* test_selection_sort.c - all three functions against qsort() on 20,000 random
 * arrays of 0 to 64 elements, with INT_MIN, INT_MAX and many duplicates. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "selection_sort.h"

static int cmp_int(const void *pa, const void *pb)
{
    int a = *(const int *)pa, b = *(const int *)pb;
    return (a > b) - (a < b);
}

int main(void)
{
    unsigned long long xs = 0x9E3779B97F4A7C15ULL;
    int in[64], ref[64], out[64], failures = 0;

    for (int t = 0; t < 20000; t++) {
        xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17;
        size_t n = (size_t)(xs % 65);
        int range = (t % 3 == 0) ? 4 : 1000;
        for (size_t i = 0; i < n; i++) {
            xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17;
            in[i] = (xs % 17 == 0) ? INT_MIN : (xs % 17 == 1) ? INT_MAX
                  : (int)(xs % (unsigned)range) - range / 2;
        }
        memcpy(ref, in, n * sizeof in[0]);
        qsort(ref, n, sizeof ref[0], cmp_int);

        static void (*const fns[])(int *, size_t) = { selection_sort, stable_selection_sort, double_selection_sort };
        static const char *names[] = { "selection_sort", "stable_selection_sort", "double_selection_sort" };
        for (int f = 0; f < 3; f++) {
            memcpy(out, in, n * sizeof in[0]);
            fns[f](n ? out : NULL, n);
            if (n && memcmp(out, ref, n * sizeof out[0]) != 0) { printf("FAIL %s case %d\n", names[f], t); failures++; }
        }
    }
    printf("%s: 3 functions x 20000 arrays of 0-64 elements\n", failures ? "FAILED" : "passed");
    return failures != 0;
}
