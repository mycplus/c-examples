/* heap_example.c - builds a heap, then sorts, with both variants */
#include <stdio.h>
#include <limits.h>
#include "heap_sort.h"

static void print_array(const char *label, const int a[], size_t n)
{
    printf("%-10s", label);
    for (size_t i = 0; i < n; i++)
        printf(" %d", a[i]);
    printf("\n");
}

int main(void)
{
    int a[] = { 29, 10, 14, 37, 13, 5, 41, 22 };
    int b[] = { 29, 10, 14, 37, 13, 5, 41, 22 };
    int h[] = { 29, 10, 14, 37, 13, 5, 41, 22 };
    int edge[] = { 0, INT_MAX, -1, INT_MIN, 0 };
    size_t n = sizeof a / sizeof a[0];

    print_array("before:", a, n);
    make_heap_down(h, n);
    print_array("max-heap:", h, n);
    heap_sort(a, n);
    print_array("sorted:", a, n);
    heap_sort_floyd(b, n);
    print_array("floyd:", b, n);
    heap_sort(edge, 5);
    print_array("limits:", edge, 5);
    heap_sort(NULL, 0);                      /* empty: no loop runs */
    return 0;
}
