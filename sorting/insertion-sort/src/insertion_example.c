/* insertion_example.c - calls both functions from insertion_sort.c */
#include <stdio.h>
#include <limits.h>
#include "insertion_sort.h"

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
    int edge[] = { 0, INT_MAX, -1, INT_MIN, 0 };
    size_t n = sizeof a / sizeof a[0];

    print_array("before:", a, n);
    insertion_sort(a, n);
    print_array("after:", a, n);
    binary_insertion_sort(b, n);
    print_array("binary:", b, n);
    insertion_sort(edge, 5);
    print_array("limits:", edge, 5);
    insertion_sort(NULL, 0);             /* empty: the loop never runs */
    return 0;
}
