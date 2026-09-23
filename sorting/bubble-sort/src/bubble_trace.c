/* bubble_trace.c - print every comparison bubble sort makes */
#include <stdio.h>
#include <stddef.h>

static void print_array(const int a[], size_t n)
{
    for (size_t i = 0; i < n; i++)
        printf("%3d", a[i]);
}

int main(void)
{
    int a[] = { 7, 3, 9, 2, 11, 15 };
    size_t n = sizeof a / sizeof a[0];
    size_t bound = n;           /* a[bound..n-1] is already in final position */
    int pass = 0;

    printf("start:          ");
    print_array(a, n);
    printf("\n");

    while (bound > 1) {
        size_t last_swap = 0;
        printf("pass %d (compares a[0..%zu])\n", ++pass, bound - 1);

        for (size_t j = 1; j < bound; j++) {
            int swapped = a[j] < a[j - 1];
            if (swapped) {
                int tmp = a[j];
                a[j] = a[j - 1];
                a[j - 1] = tmp;
                last_swap = j;
            }
            printf("  a[%zu]:a[%zu] %-4s  ", j - 1, j, swapped ? "swap" : "ok");
            print_array(a, n);
            printf("\n");
        }
        bound = last_swap;      /* 0 when nothing moved: the array is sorted */
    }
    printf("sorted after %d passes\n", pass);
    return 0;
}
