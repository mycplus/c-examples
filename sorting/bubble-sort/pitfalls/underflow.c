#include <stdio.h>
#include <stddef.h>

void bubble_sort_textbook(int a[], size_t n)
{
    for (size_t i = 0; i < n - 1; i++)          /* n == 0: n - 1 is SIZE_MAX */
        for (size_t j = 0; j < n - 1 - i; j++)
            if (a[j] > a[j + 1]) {
                int t = a[j]; a[j] = a[j + 1]; a[j + 1] = t;
            }
}

int main(void)
{
    int buf[1] = { 0 };
    bubble_sort_textbook(buf, 0);               /* an empty array */
    puts("done");
    return 0;
}
