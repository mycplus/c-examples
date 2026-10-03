/* base_case.c - with half-open ranges [lo, hi), "stop when lo >= hi" never
 * stops on a one-element range: it splits [lo, lo+1) into [lo, lo) and
 * [lo, lo+1) forever. */
#include <stdio.h>
#include <stddef.h>

static void sort_range(int a[], size_t lo, size_t hi)
{
    if (lo >= hi)                            /* should be: hi - lo < 2 */
        return;
    size_t mid = lo + (hi - lo) / 2;
    sort_range(a, lo, mid);
    sort_range(a, mid, hi);
    /* no merge needed to show the bug: the calls above never return */
}

int main(void)
{
    int a[] = { 3, 1, 2 };
    sort_range(a, 0, 3);
    printf("%d %d %d\n", a[0], a[1], a[2]);
    return 0;
}
