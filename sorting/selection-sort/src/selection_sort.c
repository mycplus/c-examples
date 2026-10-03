/* selection_sort.c - selection sort in C11.
 * SORT_LESS(x, y) means "x sorts before y" and SORT_SWAP exchanges two
 * elements. Test programs redefine them (and SORT_ON_SHIFT) before
 * including this file, to count what the algorithms do. */
#include "selection_sort.h"

static void swap_int(int *x, int *y) { int t = *x; *x = *y; *y = t; }

#ifndef SORT_LESS
#define SORT_LESS(x, y) ((x) < (y))
#endif
#ifndef SORT_SWAP
#define SORT_SWAP(x, y) swap_int(x, y)
#endif
#ifndef SORT_ON_SHIFT
#define SORT_ON_SHIFT() ((void)0)      /* lets a test program count shifts */
#endif

/* In place, at most n - 1 swaps, n(n-1)/2 comparisons. Not stable. */
void selection_sort(int a[], size_t n)
{
    for (size_t i = 0; i + 1 < n; i++) {
        size_t min = i;
        for (size_t j = i + 1; j < n; j++)
            if (SORT_LESS(a[j], a[min]))
                min = j;
        if (min != i)                    /* never swap an element with itself */
            SORT_SWAP(&a[i], &a[min]);
    }
}

/* Stable variant: instead of swapping, shift a[i..min-1] one place right
 * and put the minimum at a[i]. Same comparisons, far more moves. */
void stable_selection_sort(int a[], size_t n)
{
    for (size_t i = 0; i + 1 < n; i++) {
        size_t min = i;
        for (size_t j = i + 1; j < n; j++)
            if (SORT_LESS(a[j], a[min]))
                min = j;
        int v = a[min];
        for (size_t k = min; k > i; k--) {
            a[k] = a[k - 1];
            SORT_ON_SHIFT();
        }
        a[i] = v;
    }
}

/* Finds the minimum and the maximum in one pass and places both.
 * Half as many passes; not fewer comparisons. */
void double_selection_sort(int a[], size_t n)
{
    if (n < 2)
        return;
    size_t lo = 0, hi = n - 1;
    while (lo < hi) {
        size_t min = lo, max = lo;
        for (size_t j = lo + 1; j <= hi; j++) {
            if (SORT_LESS(a[j], a[min]))
                min = j;
            else if (SORT_LESS(a[max], a[j]))
                max = j;
        }
        if (min != lo)
            SORT_SWAP(&a[lo], &a[min]);
        if (max == lo)                   /* the max was at lo: it just moved */
            max = min;
        if (max != hi)
            SORT_SWAP(&a[hi], &a[max]);
        lo++;
        hi--;
    }
}
