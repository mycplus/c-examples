/* cache.h - a set-associative cache simulator with LRU and FIFO replacement.
 * Write-through with write-allocate: every write also goes to memory, and a
 * miss on a write loads the block, the same as a miss on a read. */
#ifndef CACHE_H
#define CACHE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum { CACHE_LRU, CACHE_FIFO } cache_policy;

typedef struct {
    uint64_t size;          /* total capacity in bytes                     */
    uint64_t block;         /* block (line) size in bytes, a power of two  */
    uint64_t ways;          /* lines per set; 1 = direct-mapped            */
    /* size must equal block * ways * sets, with sets a power of two.
     * So 48 KiB, 64-byte blocks, 12 ways (64 sets) is fine. */
    cache_policy policy;
} cache_config;

typedef struct {
    uint64_t reads, writes;           /* accesses by the program        */
    uint64_t hits, misses;            /* per block looked up            */
    uint64_t mem_reads, mem_writes;   /* traffic to the next level      */
    uint64_t evictions;               /* valid blocks replaced          */
} cache_stats;

typedef struct cache cache;

/* Returns NULL if the configuration is usable, else a message saying why not. */
const char *cache_config_error(const cache_config *cfg);

/* Returns NULL if the configuration is invalid or memory runs out. */
cache *cache_create(const cache_config *cfg);
void cache_destroy(cache *c);

/* Simulates one access of `size` bytes (at least 1) at `addr`. An access that
 * spans two blocks touches both. Returns true if every block touched hit. */
bool cache_access(cache *c, uint64_t addr, uint64_t size, bool is_write);

const cache_stats *cache_get_stats(const cache *c);
uint64_t cache_sets(const cache *c);

#endif
