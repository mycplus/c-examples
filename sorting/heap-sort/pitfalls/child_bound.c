/* child_bound.c - "child <= n" instead of "child < n" in sift_down */
#include <stdio.h>
#include <stddef.h>

static void sift_down(int a[], size_t root, size_t n)
{
    for (;;) {
        size_t child = 2 * root + 1;
        if (child > n)                       /* should be child >= n */
            return;
        if (child + 1 <= n && a[child] < a[child + 1])
            child++;
        if (!(a[root] < a[child]))
            return;
        int t = a[root]; a[root] = a[child]; a[child] = t;
        root = child;
    }
}

int main(void)
{
    int a[] = { 29, 10, 14, 37, 13, 5, 41, 22 };
    size_t n = sizeof a / sizeof a[0];
    for (size_t i = n / 2; i-- > 0; )
        sift_down(a, i, n);
    for (size_t end = n; end > 1; end--) {
        int t = a[0]; a[0] = a[end - 1]; a[end - 1] = t;
        sift_down(a, 0, end - 1);
    }
    for (size_t i = 0; i < n; i++)
        printf("%d ", a[i]);
    printf("\n");
    return 0;
}
