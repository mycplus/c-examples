/* test_bubble_sort.c - differential test of bubble_sort() against qsort() */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define main bubble_sort_demo_main      /* reuse the article's file as-is */
#include "../src/bubble_sort.c"
#undef main

static int cmp_int(const void *pa, const void *pb)
{
    int a = *(const int *)pa, b = *(const int *)pb;
    return (a > b) - (a < b);
}

static unsigned int rng = 12345u;
static unsigned int next_rand(void)
{
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return rng;
}

static int pick_value(void)
{
    switch (next_rand() % 8) {
    case 0:  return INT_MIN;
    case 1:  return INT_MAX;
    case 2:  return 0;
    case 3:  return (int)(next_rand() % 4);        /* many duplicates */
    default: return (int)next_rand();              /* full range */
    }
}

int main(void)
{
    enum { MAXN = 64, TRIALS = 20000 };
    int a[MAXN], b[MAXN];
    int failures = 0, run = 0;

    for (int t = 0; t < TRIALS; t++, run++) {
        size_t n = next_rand() % (MAXN + 1);       /* includes 0 and 1 */
        for (size_t i = 0; i < n; i++)
            a[i] = b[i] = pick_value();

        switch (t % 4) {                            /* shaped inputs too */
        case 1: qsort(a, n, sizeof a[0], cmp_int); memcpy(b, a, sizeof a); break;
        case 2: qsort(a, n, sizeof a[0], cmp_int);
                for (size_t i = 0; i < n / 2; i++) { int x = a[i]; a[i] = a[n-1-i]; a[n-1-i] = x; }
                memcpy(b, a, sizeof a); break;
        default: break;
        }

        bubble_sort(a, n);
        qsort(b, n, sizeof b[0], cmp_int);
        if (n && memcmp(a, b, n * sizeof a[0]) != 0) {
            printf("FAIL trial %d (n = %zu)\n", t, n);
            if (++failures == 5) { run++; break; }
        }
    }
    bubble_sort(NULL, 0);                           /* must not dereference */

    printf("%d trials, %d failures\n", run, failures);
    return failures != 0;
}
