/* upward_and_greedy.c - two plausible 0/1 knapsack shortcuts that return
   the wrong answer, and the capacity-scaling cost of the correct one, all on
   the four items used by src/knapsack_01.c. */
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    const char *name;
    int weight;
    long long value;
} Item;

/* Correct 0/1: capacities visited downwards. */
static long long dp_downward(const Item *it, size_t n, int cap, long long *dp)
{
    for (int c = 0; c <= cap; c++)
        dp[c] = 0;
    for (size_t i = 0; i < n; i++)
        for (int c = cap; c >= it[i].weight; c--)
            if (dp[c - it[i].weight] + it[i].value > dp[c])
                dp[c] = dp[c - it[i].weight] + it[i].value;
    return dp[cap];
}

/* Pitfall 1: capacities visited upwards. dp[c - w] may already include
   item i, so an item can be counted again: this is unbounded knapsack. */
static long long dp_upward(const Item *it, size_t n, int cap, long long *dp)
{
    for (int c = 0; c <= cap; c++)
        dp[c] = 0;
    for (size_t i = 0; i < n; i++)
        for (int c = it[i].weight; c <= cap; c++)
            if (dp[c - it[i].weight] + it[i].value > dp[c])
                dp[c] = dp[c - it[i].weight] + it[i].value;
    return dp[cap];
}

/* Pitfall 2: greedy by value/weight, whole items only. Expects the items
   already sorted by ratio, highest first. */
static long long greedy_by_ratio(const Item *it, size_t n, int cap)
{
    long long total = 0;
    for (size_t i = 0; i < n; i++)
        if (it[i].weight <= cap) {
            cap -= it[i].weight;
            total += it[i].value;
        }
    return total;
}

int main(void)
{
    /* Listed in value/weight order: 10, 6, 5, 4. */
    const Item items[] = {
        {"D", 5, 50}, {"A", 10, 60}, {"B", 20, 100}, {"C", 30, 120},
    };
    const size_t n = sizeof items / sizeof items[0];
    const int cap = 50;

    long long *dp = malloc(((size_t)cap + 1) * sizeof *dp);
    if (dp == NULL)
        return EXIT_FAILURE;

    printf("0/1 DP, capacities downward : %lld\n", dp_downward(items, n, cap, dp));
    printf("0/1 DP, capacities upward   : %lld\n", dp_upward(items, n, cap, dp));
    printf("Greedy by value/weight      : %lld\n", greedy_by_ratio(items, n, cap));

    /* Pitfall 3: the table grows with the capacity's magnitude, not the
       number of items. Scaling every weight and the capacity by k changes
       nothing about the answer and multiplies the work by k. */
    for (int k = 1; k <= 1000; k *= 10) {
        Item scaled[4];
        for (size_t i = 0; i < n; i++) {
            scaled[i] = items[i];
            scaled[i].weight *= k;
        }
        long long *big = malloc(((size_t)cap * k + 1) * sizeof *big);
        if (big == NULL)
            return EXIT_FAILURE;
        long long best = dp_downward(scaled, n, cap * k, big);
        printf("Weights x%-4d capacity %6d  best %lld  cells %zu\n",
               k, cap * k, best, n * ((size_t)cap * k + 1));
        free(big);
    }

    free(dp);
    return EXIT_SUCCESS;
}
