/* Pitfall: padding every row to the full width. On screen the two pyramids
   look the same, but the padded one ends every line with spaces. Online
   judges and tests that compare output byte for byte reject it, and so
   does "diff". Here '|' marks where each line really ends. */
#include <stdio.h>

int main(void)
{
    int n = 4;
    printf("stars, then newline (correct)\n");
    for (int i = 1; i <= n; i++) {
        for (int k = 1; k <= n - i; k++)
            putchar(' ');
        for (int k = 1; k < 2 * i; k++)
            putchar('*');
        printf("|\n");
    }

    printf("\npadded to 2n - 1 columns\n");
    for (int i = 1; i <= n; i++) {
        for (int col = 1; col <= 2 * n - 1; col++)
            putchar(col > n - i && col < n + i ? '*' : ' ');
        printf("|\n");
    }
    return 0;
}
