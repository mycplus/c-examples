/* Pitfall: one character in the loop condition. Row i of a centered pyramid
   needs 2i - 1 stars, an odd number, so the middle star of every row lines
   up. With k <= 2 * i instead of k < 2 * i the rows have 2, 4, 6 ... stars,
   and an even row has no middle: the pyramid leans to one side. */
#include <stdio.h>

static void pyramid(int n, int wrong)
{
    for (int i = 1; i <= n; i++) {
        for (int k = 1; k <= n - i; k++)
            putchar(' ');
        for (int k = 1; wrong ? k <= 2 * i : k < 2 * i; k++)
            putchar('*');
        putchar('\n');
    }
}

int main(void)
{
    printf("k < 2 * i  (correct)\n");
    pyramid(4, 0);
    printf("\nk <= 2 * i (one star too many)\n");
    pyramid(4, 1);
    return 0;
}
