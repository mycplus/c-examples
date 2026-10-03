/* xor_swap.c - selection sort with an XOR swap and no min != i check */
#include <stdio.h>
#include <stddef.h>

static void xor_swap(int *x, int *y)
{
    *x ^= *y;                            /* if x and y are the same element, */
    *y ^= *x;                            /* the first line sets it to 0      */
    *x ^= *y;
}

void selection_sort_xor(int a[], size_t n)
{
    for (size_t i = 0; i + 1 < n; i++) {
        size_t min = i;
        for (size_t j = i + 1; j < n; j++)
            if (a[j] < a[min])
                min = j;
        xor_swap(&a[i], &a[min]);        /* runs even when min == i */
    }
}

int main(void)
{
    int a[] = { 29, 10, 14, 37, 13, 5, 41, 22 };
    selection_sort_xor(a, 8);
    for (int i = 0; i < 8; i++)
        printf("%d ", a[i]);
    printf("\n");
    return 0;
}
