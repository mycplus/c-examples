/* belady.c - Belady's anomaly: with FIFO replacement, a bigger cache can
 * miss more often. LRU never does. Each address is its own one-byte block
 * in a fully associative cache, so this is the classic page-reference test. */
#include <stdio.h>

#include "cache.h"

static unsigned long long misses(cache_policy policy, uint64_t lines)
{
    static const uint64_t refs[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    const cache_config cfg = {lines, 1, lines, policy};   /* size, block, ways */
    cache *c = cache_create(&cfg);
    if (c == NULL)
        return 0;
    for (size_t i = 0; i < sizeof refs / sizeof refs[0]; i++)
        cache_access(c, refs[i], 1, false);
    const unsigned long long m = (unsigned long long)cache_get_stats(c)->misses;
    cache_destroy(c);
    return m;
}

int main(void)
{
    printf("References: 1 2 3 4 1 2 5 1 2 3 4 5\n");
    printf("Lines  FIFO misses  LRU misses\n");
    for (uint64_t lines = 1; lines <= 5; lines++)
        printf("%5llu  %11llu  %10llu\n", (unsigned long long)lines,
               misses(CACHE_FIFO, lines), misses(CACHE_LRU, lines));
    return 0;
}
