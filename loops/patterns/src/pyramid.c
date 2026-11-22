/* pyramid: asks for a number of rows and prints a centered pyramid of stars.
   Row i (counting from 1) has n - i spaces and then 2i - 1 stars. */
#include <stdio.h>

int main(void)
{
    int n;
    printf("Number of rows (1-50): ");
    if (scanf("%d", &n) != 1 || n < 1 || n > 50) {
        printf("\nPlease enter a whole number from 1 to 50.\n");
        return 1;
    }
    putchar('\n');
    for (int i = 1; i <= n; i++) {
        for (int s = 0; s < n - i; s++)          /* leading spaces */
            putchar(' ');
        for (int k = 0; k < 2 * i - 1; k++)      /* stars: 1, 3, 5, ... */
            putchar('*');
        putchar('\n');
    }
    return 0;
}
