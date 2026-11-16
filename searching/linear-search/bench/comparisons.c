/* comparisons.c - average number of comparisons per search, counted
 * exactly from the index each search returns, for 100 distinct keys in a
 * shuffled array:
 *   - every key equally likely, and keys that are not in the array;
 *   - keys asked for with Zipf frequencies (key k about 1/k as often as
 *     key 1), searched with the plain loop, move-to-front and transpose,
 *     and the best any fixed order can do. */
#include <stdint.h>
#include <stdio.h>

#include "linear_search.h"

#define N 100
#define SEARCHES 1000000

static uint64_t state = 42;
static uint64_t next_random(void)    /* splitmix64 */
{
    uint64_t z = (state += 0x9e3779b97f4a7c15u);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9u;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebu;
    return z ^ (z >> 31);
}

static void shuffled(int *a)
{
    for (int k = 0; k < N; k++)
        a[k] = k + 1;
    for (int k = N - 1; k > 0; k--) {          /* Fisher-Yates */
        int j = (int)(next_random() % (uint64_t)(k + 1)), t = a[k];
        a[k] = a[j];
        a[j] = t;
    }
}

/* Integer Zipf weights, so every platform draws the same keys. */
static uint64_t weight[N + 1], total;
static int zipf_key(void)
{
    uint64_t r = next_random() % total;
    int k = 1;
    while (r >= weight[k])
        r -= weight[k++];
    return k;
}

int main(void)
{
    int a[N];
    uint64_t sum = 0;

    shuffled(a);
    for (int s = 0; s < SEARCHES; s++)
        sum += (uint64_t)linear_search(a, N, (int)(next_random() % N) + 1) + 1;
    printf("uniform keys, found:        %6.2f comparisons (predicted (n + 1) / 2 = %.2f)\n",
           (double)sum / SEARCHES, (N + 1) / 2.0);
    printf("keys not in the array:      %6d comparisons (always n)\n", N);

    for (int k = 1; k <= N; k++)
        total += weight[k] = 1000000 / (uint64_t)k;
    double best = 0;
    for (int k = 1; k <= N; k++)            /* most frequent first */
        best += (double)k * (double)weight[k] / (double)total;

    const char *names[] = {"plain loop", "move-to-front", "transpose"};
    for (int method = 0; method < 3; method++) {
        shuffled(a);
        sum = 0;
        for (int s = 0; s < SEARCHES; s++) {
            int key = zipf_key();
            ptrdiff_t at = method == 0 ? linear_search(a, N, key)
                         : method == 1 ? search_move_to_front(a, N, key)
                                       : search_transpose(a, N, key);
            sum += (uint64_t)at + 1;
        }
        printf("Zipf keys, %-16s %6.2f comparisons\n", names[method], (double)sum / SEARCHES);
    }
    printf("Zipf keys, %-16s %6.2f comparisons\n", "best fixed order", best);
    return 0;
}
