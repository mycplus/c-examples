/* heap_demo.c - heap sort traced: the heap after it is built, then the
 * array after each extraction of the largest element */
#include <stdio.h>
#include <stddef.h>
#include "heap_sort.h"

static void print_array(const char *label, const int a[], size_t n, size_t heap_end)
{
    printf("%-22s", label);
    for (size_t i = 0; i < n; i++)
        printf(i == heap_end ? " |%3d" : "%5d", a[i]);    /* | marks the sorted tail */
    printf("\n");
}

static void sift_down(int a[], size_t root, size_t n)    /* same as heap_sort.c */
{
    for (;;) {
        size_t child = 2 * root + 1;
        if (child >= n) return;
        if (child + 1 < n && a[child] < a[child + 1]) child++;
        if (!(a[root] < a[child])) return;
        int t = a[root]; a[root] = a[child]; a[child] = t;
        root = child;
    }
}

int main(void)
{
    int a[] = { 29, 10, 14, 37, 13, 5, 41, 22 };
    size_t n = sizeof a / sizeof a[0];
    char label[32];

    print_array("start:", a, n, n);
    make_heap_down(a, n);
    print_array("max-heap built:", a, n, n);
    for (size_t end = n; end > 1; end--) {
        int t = a[0]; a[0] = a[end - 1]; a[end - 1] = t;
        sift_down(a, 0, end - 1);
        snprintf(label, sizeof label, "extract %d:", a[end - 1]);
        print_array(label, a, n, end - 1);
    }
    return 0;
}
