/* heap_sort.c - heap sort in C11, with a max-heap stored in the array:
 * the children of index i are at 2i + 1 and 2i + 2, its parent at (i - 1) / 2.
 * SORT_LESS(x, y) means "x sorts before y". Test programs redefine it
 * before including this file, to count comparisons. */
#include "heap_sort.h"

#ifndef SORT_LESS
#define SORT_LESS(x, y) ((x) < (y))
#endif

static void swap_int(int *x, int *y) { int t = *x; *x = *y; *y = t; }

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

/* Moves a[i] up until its parent is not smaller. */
static void sift_up(int a[], size_t i)
{
    while (i > 0) {
        size_t parent = (i - 1) / 2;
        if (!SORT_LESS(a[parent], a[i]))
            return;
        swap_int(&a[parent], &a[i]);
        i = parent;
    }
}

/* Floyd's bottom-up heapify: fix the subtrees from the last parent back. */
void make_heap_down(int a[], size_t n)
{
    for (size_t i = n / 2; i-- > 0; )        /* n / 2 - 1 down to 0, no wrap */
        sift_down(a, i, n);
}

/* Builds the heap by inserting a[1], a[2], ... one at a time. */
void make_heap_up(int a[], size_t n)
{
    for (size_t i = 1; i < n; i++)
        sift_up(a, i);
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

/* Floyd's variant: the element moved to the root is almost always small,
 * so walk the larger-child path all the way to a leaf without comparing
 * against it (one comparison per level instead of two), then climb back
 * up to where it belongs. */
void heap_sort_floyd(int a[], size_t n)
{
    make_heap_down(a, n);
    for (size_t end = n; end > 1; end--) {
        size_t m = end - 1;                  /* heap is a[0..m) after the swap */
        int v = a[m];
        a[m] = a[0];                         /* largest to its final place */
        size_t hole = 0;
        for (;;) {                           /* 1. descend to a leaf */
            size_t child = 2 * hole + 1;
            if (child >= m)
                break;
            if (child + 1 < m && SORT_LESS(a[child], a[child + 1]))
                child++;
            a[hole] = a[child];
            hole = child;
        }
        while (hole > 0) {                   /* 2. climb back up to v's place */
            size_t parent = (hole - 1) / 2;
            if (!SORT_LESS(a[parent], v))
                break;
            a[hole] = a[parent];
            hole = parent;
        }
        a[hole] = v;
    }
}
