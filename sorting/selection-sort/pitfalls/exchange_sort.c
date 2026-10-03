/* exchange_sort.c - swapping inside the inner loop still sorts, but it is
 * not selection sort: count the swaps on 10,000 random values. */
#include <stdio.h>
#include <stddef.h>

#define N 10000

static unsigned long long swaps;

static void swap_int(int *x, int *y) { int t = *x; *x = *y; *y = t; swaps++; }

static void exchange_sort(int a[], size_t n)       /* swap as you go */
{
    for (size_t i = 0; i + 1 < n; i++)
        for (size_t j = i + 1; j < n; j++)
            if (a[j] < a[i])
                swap_int(&a[i], &a[j]);
}

static void selection_sort(int a[], size_t n)      /* remember, swap once */
{
    for (size_t i = 0; i + 1 < n; i++) {
        size_t min = i;
        for (size_t j = i + 1; j < n; j++)
            if (a[j] < a[min])
                min = j;
        if (min != i)
            swap_int(&a[i], &a[min]);
    }
}

int main(void)
{
    static int a[N], b[N];
    unsigned long long xs = 88172645463325252ULL;
    for (size_t i = 0; i < N; i++) {
        xs ^= xs << 13; xs ^= xs >> 7; xs ^= xs << 17;
        a[i] = b[i] = (int)(xs % 1000000);
    }
    swaps = 0; exchange_sort(a, N);
    printf("exchange sort:  %llu swaps\n", swaps);
    swaps = 0; selection_sort(b, N);
    printf("selection sort: %llu swaps\n", swaps);
    for (size_t i = 0; i < N; i++)
        if (a[i] != b[i]) { printf("results differ\n"); return 1; }
    printf("both results sorted and identical\n");
    return 0;
}
