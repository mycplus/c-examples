/* test_lfind.c - linear_search must agree with the C library's lfind()
 * (POSIX <search.h>) on random arrays. Not built on Windows. */
#define _XOPEN_SOURCE 700   /* lfind() */
#include <search.h>
#include <stdint.h>
#include <stdio.h>

#include "linear_search.h"

static int cmp_int(const void *x, const void *y)
{
    return *(const int *)x != *(const int *)y;   /* lfind wants 0 for a match */
}

int main(void)
{
    uint32_t s = 7;
    int a[100], agree = 0, trials = 10000;
    for (int t = 0; t < trials; t++) {
        size_t n = (s = s * 1664525u + 1013904223u) % 101;
        for (size_t k = 0; k < n; k++)
            a[k] = (int)((s = s * 1664525u + 1013904223u) >> 24) % 50;
        int key = (int)((s = s * 1664525u + 1013904223u) >> 24) % 60;
        const int *p = lfind(&key, a, &n, sizeof a[0], cmp_int);
        ptrdiff_t want = p == NULL ? -1 : p - a;
        agree += linear_search(a, n, key) == want;
    }
    printf("random arrays where linear_search and lfind agree: %d of %d\n", agree, trials);
    return agree != trials;
}
