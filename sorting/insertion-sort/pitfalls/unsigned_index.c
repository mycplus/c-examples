/* unsigned_index.c - the textbook loop "j >= 0" with a size_t index */
#include <stdio.h>
#include <stddef.h>

void insertion_sort_unsigned(int a[], size_t n)
{
    for (size_t i = 1; i < n; i++) {
        int v = a[i];
        size_t j;
        for (j = i - 1; j >= 0 && a[j] > v; j--)   /* j >= 0 is always true */
            a[j + 1] = a[j];
        a[j + 1] = v;
    }
}

int main(void)
{
    int a[] = { 3, 1, 2 };
    insertion_sort_unsigned(a, 3);
    printf("%d %d %d\n", a[0], a[1], a[2]);
    return 0;
}
