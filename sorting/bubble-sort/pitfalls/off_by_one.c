#include <stdio.h>
#include <stddef.h>

void bubble_sort_off_by_one(int a[], size_t n)
{
    for (size_t i = 0; i + 1 < n; i++)
        for (size_t j = 0; j < n - i; j++)      /* should be j + 1 < n - i */
            if (a[j] > a[j + 1]) {
                int t = a[j]; a[j] = a[j + 1]; a[j + 1] = t;
            }
}

int main(void)
{
    int a[] = { 3, 1, 2 };
    bubble_sort_off_by_one(a, 3);
    printf("%d %d %d\n", a[0], a[1], a[2]);
    return 0;
}
