/* bubble_sort_flag.c - the classic swapped-flag version, for comparison */
#include <stddef.h>

void bubble_sort_flag(int a[], size_t n)
{
    for (size_t pass = 1; pass < n; pass++) {
        int swapped = 0;
        for (size_t j = 1; j <= n - pass; j++) {
            if (a[j] < a[j - 1]) {
                int tmp = a[j];
                a[j] = a[j - 1];
                a[j - 1] = tmp;
                swapped = 1;
            }
        }
        if (!swapped)           /* a pass with no swaps: already sorted */
            break;
    }
}
