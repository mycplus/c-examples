/* one_based.c - the 1-based child formulas 2i and 2i + 1 on a 0-based array */
#include <stdio.h>
#include <stddef.h>

static void sift_down_1based(int a[], size_t root, size_t n)
{
    for (;;) {
        size_t child = 2 * root;             /* right for a[1..n], wrong for a[0..n) */
        if (child >= n)
            return;
        if (child + 1 < n && a[child] < a[child + 1])
            child++;
        if (!(a[root] < a[child]))
            return;
        int t = a[root]; a[root] = a[child]; a[child] = t;
        root = child;
    }
}

static void heap_sort_1based(int a[], size_t n)
{
    for (size_t i = n / 2; i-- > 0; )
        sift_down_1based(a, i, n);
    for (size_t end = n; end > 1; end--) {
        int t = a[0]; a[0] = a[end - 1]; a[end - 1] = t;
        sift_down_1based(a, 0, end - 1);
    }
}

static void show(const char *label, const int a[], size_t n)
{
    printf("%s", label);
    for (size_t i = 0; i < n; i++)
        printf(" %d", a[i]);
    printf("\n");
}

int main(void)
{
    int a[] = { 29, 10, 14, 37, 13, 5, 41, 22 };   /* the article's example */
    int b[] = { 45, 9, 84, 35, 95 };               /* found by random search */
    heap_sort_1based(a, 8);
    heap_sort_1based(b, 5);
    show("example:", a, 8);
    show("other:  ", b, 5);
    return 0;
}
