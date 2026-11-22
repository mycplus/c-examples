/* subtract_compare.c - a qsort comparator written as a subtraction. It works
 * for small costs, but a - b overflows when the costs are far apart, and
 * then the edges come out in the wrong order. */
#include <stdio.h>
#include <stdlib.h>

typedef struct { int u, v, weight; } Edge;

static int by_weight_subtract(const void *x, const void *y)
{
    return ((const Edge *)x)->weight - ((const Edge *)y)->weight;   /* overflows */
}

static int by_weight_compare(const void *x, const void *y)
{
    int a = ((const Edge *)x)->weight, b = ((const Edge *)y)->weight;
    return (a > b) - (a < b);
}

static void show(const char *label, int (*cmp)(const void *, const void *))
{
    Edge e[] = {{0, 1, 2000000000}, {1, 2, -2000000000}, {0, 2, 5}};
    qsort(e, 3, sizeof e[0], cmp);
    printf("%-11s", label);
    for (int i = 0; i < 3; i++)
        printf(" %11d", e[i].weight);
    printf("\n");
}

int main(void)
{
    show("subtract:", by_weight_subtract);
    show("compare:", by_weight_compare);
    return 0;
}
