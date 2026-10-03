/* unsequenced.c - reading and modifying j in one expression */
#include <stdio.h>

void insertion_sort_unsequenced(int a[], int n)
{
    for (int i = 1; i < n; i++) {
        int v = a[i];
        int j = i;
        while (j > 0 && a[j - 1] > v)
            a[j--] = a[j - 1];                     /* undefined behavior */
        a[j] = v;
    }
}

int main(void)
{
    int a[] = { 29, 10, 14, 37, 13, 5, 41, 22 };
    insertion_sort_unsequenced(a, 8);
    for (int i = 0; i < 8; i++)
        printf("%d ", a[i]);
    printf("\n");
    return 0;
}
