#include <stdio.h>
#include <stdlib.h>

typedef struct {
    const char *name;
    int weight;      /* must be > 0 */
    long long value; /* must be >= 0 */
} Item;

/* Higher value per unit of weight first. Compares v1/w1 with v2/w2 as
   v1*w2 against v2*w1, which is exact for weights and values small enough
   that the products fit in a long long. */
static int by_ratio_desc(const void *pa, const void *pb)
{
    const Item *a = pa, *b = pb;
    long long lhs = a->value * b->weight;
    long long rhs = b->value * a->weight;
    return (lhs < rhs) - (lhs > rhs);
}

/* Fractional knapsack: items may be split. Sorts items in place, then
   sets share[i] to the fraction (0.0 to 1.0) of sorted item i that is taken.
   Returns the best value, or -1.0 for invalid input. */
double knapsack_fractional(Item *items, size_t n, int capacity, double *share)
{
    if (capacity < 0 || (n > 0 && (items == NULL || share == NULL)))
        return -1.0;
    for (size_t i = 0; i < n; i++)
        if (items[i].weight <= 0 || items[i].value < 0)
            return -1.0;

    qsort(items, n, sizeof *items, by_ratio_desc);

    double total = 0.0;
    int room = capacity;
    for (size_t i = 0; i < n; i++) {
        if (items[i].weight <= room) {
            share[i] = 1.0;
            room -= items[i].weight;
        } else {
            share[i] = (double)room / items[i].weight;
            room = 0;
        }
        total += share[i] * (double)items[i].value;
    }
    return total;
}

int main(void)
{
    Item items[] = {
        {"A", 10, 60}, {"B", 20, 100}, {"C", 30, 120}, {"D", 5, 50},
    };
    const size_t n = sizeof items / sizeof items[0];

    double share[sizeof items / sizeof items[0]];

    double best = knapsack_fractional(items, n, 50, share);
    if (best < 0.0) {
        fputs("invalid input\n", stderr);
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < n; i++)
        printf("  %s  value/weight %5.2f  taken %3.0f%%\n", items[i].name,
               (double)items[i].value / items[i].weight, share[i] * 100.0);
    printf("Fractional best value: %.2f\n", best);
    return EXIT_SUCCESS;
}
