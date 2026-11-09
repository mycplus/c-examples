#include <stdio.h>
#include <stddef.h>

int main(void)
{
    int a[5] = {10, 20, 30, 40, 50};
    int *p = a;                          /* a decays to &a[0] */

    printf("sizeof(a) = %zu, sizeof(p) = %zu\n", sizeof a, sizeof p);
    printf("a[3] = %d, *(a + 3) = %d, *(p + 3) = %d\n", a[3], *(a + 3), *(p + 3));
    printf("&a[3] - &a[0] = %td elements\n", &a[3] - &a[0]);

    int *q = a;
    int v1 = *q++;                       /* read a[0], then advance q */
    printf("*q++   gave %d, q is now at index %td\n", v1, q - a);

    int v2 = (*q)++;                     /* increment the value a[1] itself */
    printf("(*q)++ gave %d, a[1] is now %d, q is still at index %td\n", v2, a[1], q - a);

    int v3 = *++q;                       /* advance q first, then read */
    printf("*++q   gave %d, q is now at index %td\n", v3, q - a);
    return 0;
}
