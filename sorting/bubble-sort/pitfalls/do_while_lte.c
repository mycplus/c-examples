/* do_while_lte.c - with <= and a do/while(swapped) loop, duplicates never settle */
#include <stdio.h>
#include <stddef.h>
static void sort_lte(int a[], size_t n)
{
    int swapped;
    long passes = 0;
    do {
        swapped = 0;
        for (size_t j = 1; j < n; j++)
            if (a[j] <= a[j - 1]) { int t = a[j]; a[j] = a[j - 1]; a[j - 1] = t; swapped = 1; }
        if (++passes == 1000000) { printf("still swapping after %ld passes\n", passes); return; }
    } while (swapped);
    printf("finished after %ld passes\n", passes);
}
int main(void)
{
    int a[] = { 3, 1, 3 };
    int b[] = { 3, 1, 2 };
    sort_lte(b, 3);
    sort_lte(a, 3);
    return 0;
}
