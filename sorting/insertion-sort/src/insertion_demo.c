/* insertion_demo.c - insertion sort traced step by step, plus edge cases */
#include <stdio.h>
#include <limits.h>
#include "insertion_sort.h"

static void print_array(const char *label, const int a[], size_t n)
{
    printf("%-22s", label);
    for (size_t i = 0; i < n; i++)
        printf("%4d", a[i]);
    printf("\n");
}

int main(void)
{
    int a[] = { 29, 10, 14, 37, 13, 5, 41, 22 };
    size_t n = sizeof a / sizeof a[0];

    print_array("start:", a, n);
    for (size_t i = 1; i < n; i++) {         /* the loop of insertion_sort, */
        int v = a[i];                        /* one step at a time          */
        size_t j = i;
        while (j > 0 && v < a[j - 1]) {
            a[j] = a[j - 1];
            j--;
        }
        a[j] = v;
        char label[32];
        snprintf(label, sizeof label, "insert %d (%zu moved):", v, i - j);
        print_array(label, a, n);
    }

    int b[] = { 29, 10, 14, 37, 13, 5, 41, 22 };
    binary_insertion_sort(b, n);
    print_array("binary insertion:", b, n);

    int edge[] = { 0, INT_MAX, -1, INT_MIN, 0 };
    insertion_sort(edge, 5);
    printf("limits: %d %d %d %d %d\n", edge[0], edge[1], edge[2], edge[3], edge[4]);
    insertion_sort(NULL, 0);                 /* empty: the loop never runs */
    printf("empty array: ok\n");
    return 0;
}
