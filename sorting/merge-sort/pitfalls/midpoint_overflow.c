/* midpoint_overflow.c - (lo + hi) / 2 with int indices near INT_MAX.
 * Needs no huge array: the arithmetic is computed before any access. */
#include <stdio.h>

static int mid_sum(int lo, int hi)  { return (lo + hi) / 2; }        /* overflows */
static int mid_diff(int lo, int hi) { return lo + (hi - lo) / 2; }   /* does not  */

int main(void)
{
    int lo = 1500000000, hi = 2000000000;    /* a range inside a 2-billion array */
    printf("(lo + hi) / 2      = %d\n", mid_sum(lo, hi));
    printf("lo + (hi - lo) / 2 = %d\n", mid_diff(lo, hi));
    return 0;
}
