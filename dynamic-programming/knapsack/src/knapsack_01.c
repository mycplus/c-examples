#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    const char *name;
    int weight;
    long long value;
} Item;

/* Rejects negative weights or values, and inputs whose total value
   would not fit in a long long (so no sum below can overflow). */
static bool items_valid(const Item *items, size_t n)
{
    long long total = 0;
    for (size_t i = 0; i < n; i++) {
        if (items[i].weight < 0 || items[i].value < 0)
            return false;
        if (items[i].value > LLONG_MAX - total)
            return false;
        total += items[i].value;
    }
    return true;
}

/* Best total value for 0/1 knapsack using one row of size capacity + 1.
   Returns 0 on success, -1 for invalid input, -2 if memory runs out. */
int knapsack_value(const Item *items, size_t n, int capacity, long long *best)
{
    if (capacity < 0 || (n > 0 && items == NULL) || best == NULL)
        return -1;
    if (!items_valid(items, n))
        return -1;

    long long *dp = calloc((size_t)capacity + 1, sizeof *dp);
    if (dp == NULL)
        return -2;

    for (size_t i = 0; i < n; i++) {
        int wt = items[i].weight;
        long long val = items[i].value;
        /* Downwards, so dp[c - wt] still holds the value without item i. */
        for (int c = capacity; c >= wt; c--) {
            if (dp[c - wt] + val > dp[c])
                dp[c] = dp[c - wt] + val;
        }
    }

    *best = dp[capacity];
    free(dp);
    return 0;
}

/* Same answer, plus which items achieve it. take[i] is set to true for
   every chosen item. Uses one byte per (item, capacity) cell. */
int knapsack_select(const Item *items, size_t n, int capacity,
                    long long *best, bool *take)
{
    if (capacity < 0 || (n > 0 && (items == NULL || take == NULL)) || best == NULL)
        return -1;
    if (!items_valid(items, n))
        return -1;

    size_t cols = (size_t)capacity + 1;
    if (n > 0 && cols > SIZE_MAX / n)
        return -2;

    long long *dp = calloc(cols, sizeof *dp);
    unsigned char *kept = calloc(n * cols, 1);
    if (dp == NULL || (n > 0 && kept == NULL)) {
        free(dp);
        free(kept);
        return -2;
    }

    for (size_t i = 0; i < n; i++) {
        int wt = items[i].weight;
        long long val = items[i].value;
        for (int c = capacity; c >= wt; c--) {
            if (dp[c - wt] + val > dp[c]) {
                dp[c] = dp[c - wt] + val;
                kept[i * cols + (size_t)c] = 1;
            }
        }
    }

    /* Walk back from the last item: if item i improved capacity c, it is in. */
    int c = capacity;
    for (size_t i = n; i-- > 0;) {
        take[i] = kept[i * cols + (size_t)c];
        if (take[i])
            c -= items[i].weight;
    }

    *best = dp[capacity];
    free(dp);
    free(kept);
    return 0;
}

int main(void)
{
    const Item items[] = {
        {"A", 10, 60}, {"B", 20, 100}, {"C", 30, 120}, {"D", 5, 50},
    };
    const size_t n = sizeof items / sizeof items[0];
    const int capacity = 50;

    long long best;
    bool take[sizeof items / sizeof items[0]];

    if (knapsack_select(items, n, capacity, &best, take) != 0) {
        fputs("knapsack_select failed\n", stderr);
        return EXIT_FAILURE;
    }

    int used = 0;
    printf("Capacity %d, best value %lld\n", capacity, best);
    for (size_t i = 0; i < n; i++) {
        if (take[i]) {
            printf("  take %s  weight %2d  value %3lld\n",
                   items[i].name, items[i].weight, items[i].value);
            used += items[i].weight;
        }
    }
    printf("Weight used: %d of %d\n", used, capacity);

    long long check;
    if (knapsack_value(items, n, capacity, &check) != 0 || check != best) {
        fputs("knapsack_value disagrees\n", stderr);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
