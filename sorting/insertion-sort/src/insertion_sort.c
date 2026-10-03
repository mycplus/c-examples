/* insertion_sort.c - insertion sort in C11.
 * SORT_LESS(x, y) means "x sorts before y". Test programs redefine it
 * before including this file, to count comparisons. */
#include <string.h>
#include "insertion_sort.h"

#ifndef SORT_LESS
#define SORT_LESS(x, y) ((x) < (y))
#endif

/* Stable, in place. O(n) on sorted input, O(n^2) worst case. */
void insertion_sort(int a[], size_t n)
{
    for (size_t i = 1; i < n; i++) {
        int v = a[i];                        /* the element being inserted */
        size_t j = i;
        while (j > 0 && SORT_LESS(v, a[j - 1])) {
            a[j] = a[j - 1];                 /* shift the larger one right */
            j--;
        }
        a[j] = v;
    }
}

/* Binary insertion sort: finds each insertion point by binary search,
 * then shifts with memmove. Fewer comparisons, the same element moves.
 * Searching for the first element greater than v keeps it stable. */
void binary_insertion_sort(int a[], size_t n)
{
    for (size_t i = 1; i < n; i++) {
        int v = a[i];
        size_t lo = 0, hi = i;               /* insertion point is in [lo, hi] */
        while (lo < hi) {
            size_t mid = lo + (hi - lo) / 2;
            if (SORT_LESS(v, a[mid]))
                hi = mid;
            else
                lo = mid + 1;
        }
        memmove(&a[lo + 1], &a[lo], (i - lo) * sizeof a[0]);
        a[lo] = v;
    }
}
