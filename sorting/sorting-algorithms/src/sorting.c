/* sorting.c - seven sorting algorithms (eight functions) for int arrays, in C11.
 *
 * Each function is the C code from its MYCPLUS article; only the
 * comparisons have been routed through SORT_LESS(x, y), meaning "x sorts
 * before y" (plain < by default). The counting and stability programs
 * redefine it before including this file. Shell sort's article prints each
 * gap; that printing is left out here. The quicksort functions keep the
 * article's int indices; above INT_MAX elements the wrappers use heap_sort.
 */
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "sorting.h"

#ifndef SORT_LESS
#define SORT_LESS(x, y) ((x) < (y))
#endif
#ifndef MERGE_SKIP_SORTED
#define MERGE_SKIP_SORTED 1
#endif

static void swap_int(int *x, int *y) { int t = *x; *x = *y; *y = t; }

#ifndef SORT_SWAP
#define SORT_SWAP(x, y) swap_int(x, y)
#endif

/* ---- insertion sort (insertion-sort article) ---- */
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

/* ---- selection sort (selection-sort article) ---- */
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

/* ---- bubble sort (bubble-sort article) ---- */
/* Sorts a[0..n-1] in ascending order. Stable, in place, O(1) extra memory. */
void bubble_sort(int a[], size_t n)
{
    size_t bound = n;               /* a[bound..n-1] is in its final position */

    while (bound > 1) {
        size_t last_swap = 0;

        for (size_t j = 1; j < bound; j++) {
            if (SORT_LESS(a[j], a[j - 1])) {  /* strict <: equal values never swap */
                int tmp = a[j];
                a[j] = a[j - 1];
                a[j - 1] = tmp;
                last_swap = j;
            }
        }
        bound = last_swap;          /* no swaps -> 0 -> loop ends */
    }
}

/* ---- Shell sort (shell-sort-algorithm article), Knuth's gaps ---- */
void shell_sort(int a[], size_t n)
{
    size_t gap = 1;
    while (gap < n / 3)
        gap = gap * 3 + 1;              /* 1, 4, 13, 40, ... */

    for (; gap > 0; gap = (gap - 1) / 3) {
        for (size_t i = gap; i < n; i++) {   /* gapped insertion sort */
            int tmp = a[i];
            size_t j;
            for (j = i; j >= gap && SORT_LESS(tmp, a[j - gap]); j -= gap)
                a[j] = a[j - gap];      /* shift larger elements right */
            a[j] = tmp;
        }
    }
}

/* ---- merge sort (merge-sort article), top-down ---- */
/* Merges the sorted runs a[lo..mid) and a[mid..hi) through buf. On a tie
 * the left element goes first, which is what makes the sort stable. */
static void merge(int a[], int buf[], size_t lo, size_t mid, size_t hi)
{
    size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi)
        buf[k++] = SORT_LESS(a[j], a[i]) ? a[j++] : a[i++];
    while (i < mid) buf[k++] = a[i++];
    while (j < hi)  buf[k++] = a[j++];
    memcpy(a + lo, buf + lo, (hi - lo) * sizeof a[0]);
}

/* Sorts the half-open range a[lo..hi). */
static void merge_sort_rec(int a[], int buf[], size_t lo, size_t hi)
{
    if (hi - lo < 2)                         /* 0 or 1 element: sorted */
        return;
    size_t mid = lo + (hi - lo) / 2;         /* cannot overflow */
    merge_sort_rec(a, buf, lo, mid);
    merge_sort_rec(a, buf, mid, hi);
    if (MERGE_SKIP_SORTED && !SORT_LESS(a[mid], a[mid - 1]))
        return;                              /* halves already in order */
    merge(a, buf, lo, mid, hi);
}

int merge_sort(int a[], size_t n)
{
    if (n < 2)
        return 0;
    int *buf = malloc(n * sizeof *buf);      /* one buffer for every merge */
    if (buf == NULL)
        return -1;
    merge_sort_rec(a, buf, 0, n);
    free(buf);
    return 0;
}

/* ---- heap sort (heap-sort article) ---- */
/* Moves a[root] down until neither child is larger. The heap is a[0..n). */
static void sift_down(int a[], size_t root, size_t n)
{
    for (;;) {
        size_t child = 2 * root + 1;
        if (child >= n)
            return;                          /* root is a leaf */
        if (child + 1 < n && SORT_LESS(a[child], a[child + 1]))
            child++;                         /* the larger child */
        if (!SORT_LESS(a[root], a[child]))
            return;                          /* heap order holds */
        swap_int(&a[root], &a[child]);
        root = child;
    }
}

/* Floyd's bottom-up heapify: fix the subtrees from the last parent back. */
void make_heap_down(int a[], size_t n)
{
    for (size_t i = n / 2; i-- > 0; )        /* n / 2 - 1 down to 0, no wrap */
        sift_down(a, i, n);
}

/* In place, O(n log n) worst case, not stable. */
void heap_sort(int a[], size_t n)
{
    make_heap_down(a, n);
    for (size_t end = n; end > 1; end--) {
        swap_int(&a[0], &a[end - 1]);        /* largest to its final place */
        sift_down(a, 0, end - 1);
    }
}

/* ---- quicksort (quicksort-algorithm article) ---- */

/* Hoare partition: two pointers moving inward, middle pivot */
static int hoare_partition(int a[], int lo, int hi)
{
    int pivot = a[lo + (hi - lo) / 2];    /* middle element, avoids overflow */
    int i = lo - 1, j = hi + 1;
    for (;;) {
        do { i++; } while (SORT_LESS(a[i], pivot));
        do { j--; } while (SORT_LESS(pivot, a[j]));
        if (i >= j) return j;             /* a split point, NOT a pivot index */
        swap_int(&a[i], &a[j]);
    }
}

static void quicksort_hoare(int a[], int lo, int hi)
{
    if (lo < hi) {
        int p = hoare_partition(a, lo, hi);
        quicksort_hoare(a, lo, p);        /* p is included on the left */
        quicksort_hoare(a, p + 1, hi);
    }
}

/* Lomuto partition: pivot is the last element */
static int lomuto_partition(int a[], int lo, int hi)
{
    int pivot = a[hi];
    int i = lo - 1;                       /* boundary of the "smaller" region */
    for (int j = lo; j < hi; j++)
        if (SORT_LESS(a[j], pivot))
            swap_int(&a[++i], &a[j]);
    swap_int(&a[i + 1], &a[hi]);          /* put the pivot in its final place */
    return i + 1;                         /* pivot index */
}

/* Recurse into the smaller side, loop on the larger: O(log n) stack */
static void quicksort_safe(int a[], int lo, int hi)
{
    while (lo < hi) {
        int p = lomuto_partition(a, lo, hi);
        if (p - lo < hi - p) {          /* recurse into the smaller side */
            quicksort_safe(a, lo, p - 1);
            lo = p + 1;                   /* loop on the larger */
        } else {
            quicksort_safe(a, p + 1, hi);
            hi = p - 1;
        }
    }
}

void quick_sort(int a[], size_t n)
{
    if (n > (size_t)INT_MAX)
        heap_sort(a, n);                  /* beyond int indices: still sort */
    else if (n > 1)
        quicksort_hoare(a, 0, (int)n - 1);
}

void quick_sort_last(int a[], size_t n)
{
    if (n > (size_t)INT_MAX)
        heap_sort(a, n);                  /* beyond int indices: still sort */
    else if (n > 1)
        quicksort_safe(a, 0, (int)n - 1);
}
