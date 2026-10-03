/* test_heap_sort.c - both sorts and both heap builds against qsort() on 20,000 random
 * arrays of 0 to 64 elements, with INT_MIN, INT_MAX and many duplicates. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "heap_sort.h"

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

        memcpy(out, in, n * sizeof in[0]);
        heap_sort(n ? out : NULL, n);
        if (n && memcmp(out, ref, n * sizeof out[0]) != 0) { printf("FAIL heap_sort case %d\n", t); failures++; }

        memcpy(out, in, n * sizeof in[0]);
        heap_sort_floyd(n ? out : NULL, n);
        if (n && memcmp(out, ref, n * sizeof out[0]) != 0) { printf("FAIL heap_sort_floyd case %d\n", t); failures++; }

        for (int v = 0; v < 2; v++) {                 /* both builds give a valid heap */
            memcpy(out, in, n * sizeof in[0]);
            if (v == 0) make_heap_down(n ? out : NULL, n); else make_heap_up(n ? out : NULL, n);
            for (size_t i = 1; i < n; i++)
                if (out[(i - 1) / 2] < out[i]) { printf("FAIL make_heap_%s case %d\n", v ? "up" : "down", t); failures++; break; }
        }
    }
    printf("%s: 4 functions x 20000 arrays of 0-64 elements\n", failures ? "FAILED" : "passed");
    return failures != 0;
}
