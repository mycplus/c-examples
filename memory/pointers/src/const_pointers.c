#include <stdio.h>

int main(void)
{
    int a = 1, b = 2;

    const int *reader = &a;         /* pointer to const int: *reader is read-only */
    int *const anchor = &a;         /* const pointer: it can never point elsewhere */
    const int *const fixed = &a;    /* both are locked */

    reader = &b;                    /* fine: retarget the pointer */
    *anchor = 10;                   /* fine: write through the pointer */

    printf("reader sees %d, anchor wrote a = %d, fixed sees %d\n", *reader, a, *fixed);
    return 0;
}
