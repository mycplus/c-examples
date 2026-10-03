/* selection_example.c - calls selection_sort() and its two variants */
#include <stdio.h>
#include <limits.h>
#include "selection_sort.h"

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
    int c[] = { 29, 10, 14, 37, 13, 5, 41, 22 };
    int edge[] = { 0, INT_MAX, -1, INT_MIN, 0 };
    size_t n = sizeof a / sizeof a[0];

    print_array("before:", a, n);
    selection_sort(a, n);
    print_array("after:", a, n);
    stable_selection_sort(b, n);
    print_array("stable:", b, n);
    double_selection_sort(c, n);
    print_array("double:", c, n);
    selection_sort(edge, 5);
    print_array("limits:", edge, 5);
    selection_sort(NULL, 0);             /* empty: i + 1 < n is false */
    return 0;
}
