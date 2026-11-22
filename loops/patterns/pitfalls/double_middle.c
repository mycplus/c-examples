/* Pitfall: building a diamond from a pyramid and an inverted pyramid that
   both include the widest row. The middle row is printed twice, so the
   diamond has 2n rows instead of 2n - 1 and a flat middle. */
#include <stdio.h>

static void row(int n, int i)
{
    for (int k = 1; k <= n - i; k++)
        putchar(' ');
    for (int k = 1; k < 2 * i; k++)
        putchar('*');
    putchar('\n');
}

int main(void)
{
    int n = 4;
    printf("lower half from n - 1 (correct)\n");
    for (int i = 1; i <= n; i++)
        row(n, i);
    for (int i = n - 1; i >= 1; i--)
        row(n, i);

    printf("\nlower half from n (middle row twice)\n");
    for (int i = 1; i <= n; i++)
        row(n, i);
    for (int i = n; i >= 1; i--)
        row(n, i);
    return 0;
}
