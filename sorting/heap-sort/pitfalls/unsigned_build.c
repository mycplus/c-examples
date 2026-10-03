/* unsigned_build.c - the textbook build loop with a size_t index */
#include <stdio.h>
#include <stddef.h>

static void sift_down(int a[], size_t root, size_t n)
{
    for (;;) {
        size_t child = 2 * root + 1;
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

int main(void)
{
    int a[] = { 29, 10, 14, 37, 13, 5, 41, 22 };
    size_t n = sizeof a / sizeof a[0];
    for (size_t i = n / 2 - 1; i >= 0; i--)  /* i >= 0 is always true */
        sift_down(a, i, n);
    printf("heap built: %d\n", a[0]);
    return 0;
}
