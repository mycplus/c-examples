#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

static int cmp_subtract(const void *pa, const void *pb)
{
    return *(const int *)pa - *(const int *)pb;     /* overflows */
}

static int cmp_safe(const void *pa, const void *pb)
{
    int a = *(const int *)pa, b = *(const int *)pb;
    return (a > b) - (a < b);
}

int main(void)
{
    int x[] = { 1, INT_MIN, 0, INT_MAX, -1 };
    int y[] = { 1, INT_MIN, 0, INT_MAX, -1 };
    size_t n = sizeof x / sizeof x[0];

    qsort(x, n, sizeof x[0], cmp_subtract);
    qsort(y, n, sizeof y[0], cmp_safe);

    printf("subtract:");
    for (size_t i = 0; i < n; i++) printf(" %d", x[i]);
    printf("\ncompare: ");
    for (size_t i = 0; i < n; i++) printf(" %d", y[i]);
    printf("\n");
    return 0;
}
