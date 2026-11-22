/* Pitfall: ignoring scanf's return value. When the input is not a number,
   scanf("%d") returns 0, reads nothing and leaves n unchanged, so the
   program carries on with whatever n held before. Here that is 5, so
   typing "four" prints a 5-row pyramid without any error.
   (Had n not been initialised at all, reading it would be undefined behavior.)

   usage: echo four | pitfall_unchecked_scanf */
#include <stdio.h>

int main(void)
{
    int n = 5;                              /* a "default" left from earlier code */
    int got = scanf("%d", &n);
    printf("scanf returned %d, n is %d\n", got, n);
    for (int i = 1; i <= n; i++) {
        for (int k = 1; k <= n - i; k++)
            putchar(' ');
        for (int k = 1; k < 2 * i; k++)
            putchar('*');
        putchar('\n');
    }
    return 0;
}
