/* bubble_sort.c - bubble sort in C with early exit */
#include <stdio.h>
#include <stddef.h>
#include <limits.h>

/* Sorts a[0..n-1] in ascending order. Stable, in place, O(1) extra memory. */
void bubble_sort(int a[], size_t n)
{
    size_t bound = n;               /* a[bound..n-1] is in its final position */

    while (bound > 1) {
        size_t last_swap = 0;

        for (size_t j = 1; j < bound; j++) {
            if (a[j] < a[j - 1]) {  /* strict <: equal values never swap */
                int tmp = a[j];
                a[j] = a[j - 1];
                a[j - 1] = tmp;
                last_swap = j;
            }
        }
        bound = last_swap;          /* no swaps -> 0 -> loop ends */
    }
}

static void print_array(const char *label, const int a[], size_t n)
{
    printf("%-10s", label);
    for (size_t i = 0; i < n; i++)
        printf(" %d", a[i]);
    printf("\n");
}

int main(void)
{
    int data[] = { 7, 3, 9, 2, 11, 15 };
    int edge[] = { 0, INT_MAX, -1, INT_MIN, 0, 42 };
    int one[]  = { 5 };

    size_t n_data = sizeof data / sizeof data[0];
    size_t n_edge = sizeof edge / sizeof edge[0];

    print_array("before:", data, n_data);
    bubble_sort(data, n_data);
    print_array("after:", data, n_data);

    bubble_sort(edge, n_edge);
    print_array("limits:", edge, n_edge);

    bubble_sort(one, 1);            /* one element: nothing to do */
    bubble_sort(NULL, 0);           /* empty: the loop never runs  */
    print_array("single:", one, 1);
    return 0;
}
