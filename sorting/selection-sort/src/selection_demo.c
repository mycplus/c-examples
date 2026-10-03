/* selection_demo.c - selection sort traced one pass at a time */
#include <stdio.h>
#include <stddef.h>

static void print_array(const char *label, const int a[], size_t n)
{
    printf("%-26s", label);
    for (size_t i = 0; i < n; i++)
        printf("%4d", a[i]);
    printf("\n");
}

int main(void)
{
    int a[] = { 29, 10, 14, 37, 13, 5, 41, 22 };
    size_t n = sizeof a / sizeof a[0];
    char label[40];

    print_array("start:", a, n);
    for (size_t i = 0; i + 1 < n; i++) {     /* the loop of selection_sort */
        size_t min = i;
        for (size_t j = i + 1; j < n; j++)
            if (a[j] < a[min])
                min = j;
        if (min != i) {
            int t = a[i]; a[i] = a[min]; a[min] = t;
            snprintf(label, sizeof label, "pass %zu: min %d, swap:", i + 1, a[i]);
        } else {
            snprintf(label, sizeof label, "pass %zu: min %d, no swap:", i + 1, a[i]);
        }
        print_array(label, a, n);
    }
    return 0;
}
