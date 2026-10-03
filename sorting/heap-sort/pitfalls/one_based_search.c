/* one_based_search.c - how often does the 1-based child formula fail?
 * Sorts 100,000 random arrays of 2 to 16 values in [0, 100) with it and
 * counts the results that are not sorted. */
#include <stdio.h>
#include <stddef.h>

static void sift_down_1based(int a[], size_t root, size_t n)
{
    for (;;) {
        size_t child = 2 * root;             /* the 1-based formula */
        if (child >= n)
            return;
        if (child + 1 < n && a[child] < a[child + 1])
            child++;
        if (!(a[root] < a[child]))
            return;
        int t = a[root]; a[root] = a[child]; a[child] = t;
        root = child;
    }
}

int main(void)
{
    unsigned long long xs = 88172645463325252ULL;
    int failures = 0;
    for (int t = 0; t < 100000; t++) {
        int a[16];
        size_t n = 2 + (size_t)(t % 15);
        for (size_t i = 0; i < n; i++) {
            xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17;
            a[i] = (int)(xs % 100);
        }
        for (size_t i = n / 2; i-- > 0; )
            sift_down_1based(a, i, n);
        for (size_t end = n; end > 1; end--) {
            int tmp = a[0]; a[0] = a[end - 1]; a[end - 1] = tmp;
            sift_down_1based(a, 0, end - 1);
        }
        for (size_t i = 1; i < n; i++)
            if (a[i] < a[i - 1]) { failures++; break; }
    }
    printf("%d of 100000 random arrays not sorted\n", failures);
    return 0;
}
