/* cache.c - set-associative cache with LRU or FIFO replacement. */
#include "cache.h"

#include <stdlib.h>

typedef struct {
    uint64_t block;     /* full block number: address >> offset bits       */
    uint64_t stamp;     /* LRU: last use; FIFO: time the block was loaded  */
    bool valid;
} line;

struct cache {
    cache_config cfg;
    uint64_t sets;
    unsigned offset_bits;
    uint64_t clock;     /* increments on every stamp, so stamps are unique */
    cache_stats stats;
    line *lines;        /* sets * ways lines; set s starts at s * ways      */
};

static bool is_power_of_two(uint64_t x)
{
    return x != 0 && (x & (x - 1)) == 0;
}

static unsigned log2_exact(uint64_t x)  /* x is a power of two */
{
    unsigned n = 0;
    while (x > 1) {
        x >>= 1;
        n++;
    }
    return n;
}

const char *cache_config_error(const cache_config *cfg)
{
    if (!is_power_of_two(cfg->block))
        return "block size must be a power of two";
    if (cfg->ways == 0)
        return "associativity must be at least 1";
    if (cfg->size == 0 || cfg->size % cfg->block != 0)
        return "cache size must be a whole number of blocks";
    if ((cfg->size / cfg->block) % cfg->ways != 0)
        return "the number of blocks must be a multiple of the associativity";
    if (!is_power_of_two(cfg->size / cfg->block / cfg->ways))
        return "the number of sets (size / block / ways) must be a power of two";
    if (cfg->policy != CACHE_LRU && cfg->policy != CACHE_FIFO)
        return "unknown replacement policy";
    return NULL;
}

cache *cache_create(const cache_config *cfg)
{
    if (cache_config_error(cfg) != NULL)
        return NULL;
    cache *c = calloc(1, sizeof *c);
    if (c == NULL)
        return NULL;
    c->cfg = *cfg;
    c->sets = cfg->size / cfg->block / cfg->ways;
    c->offset_bits = log2_exact(cfg->block);
    /* calloc: every line starts invalid. */
    c->lines = calloc((size_t)(c->sets * cfg->ways), sizeof *c->lines);
    if (c->lines == NULL) {
        free(c);
        return NULL;
    }
    return c;
}

void cache_destroy(cache *c)
{
    if (c != NULL) {
        free(c->lines);
        free(c);
    }
}

/* Looks up one block; loads it on a miss. Returns true on a hit. */
static bool access_block(cache *c, uint64_t block)
{
    line *set = c->lines + (size_t)((block & (c->sets - 1)) * c->cfg.ways);
    line *victim = NULL;

    for (uint64_t i = 0; i < c->cfg.ways; i++) {
        line *l = &set[i];
        if (l->valid && l->block == block) {
            if (c->cfg.policy == CACHE_LRU)
                l->stamp = ++c->clock;            /* FIFO ignores hits */
            c->stats.hits++;
            return true;
        }
        /* Prefer an empty line; otherwise the oldest stamp. */
        if (victim == NULL || (victim->valid &&
                               (!l->valid || l->stamp < victim->stamp)))
            victim = l;
    }

    c->stats.misses++;
    c->stats.mem_reads++;                         /* load the block */
    if (victim->valid)
        c->stats.evictions++;
    victim->valid = true;
    victim->block = block;
    victim->stamp = ++c->clock;
    return false;
}

bool cache_access(cache *c, uint64_t addr, uint64_t size, bool is_write)
{
    if (size == 0)
        size = 1;
    if (is_write) {
        c->stats.writes++;
        c->stats.mem_writes++;                    /* write-through */
    } else {
        c->stats.reads++;
    }
    const uint64_t first = addr >> c->offset_bits;
    const uint64_t end = addr + (size - 1) < addr ? UINT64_MAX : addr + (size - 1);
    const uint64_t last = end >> c->offset_bits;
    bool hit = true;
    for (uint64_t b = first;; b++) {
        if (!access_block(c, b))
            hit = false;
        if (b == last)
            break;
    }
    return hit;
}

const cache_stats *cache_get_stats(const cache *c)
{
    return &c->stats;
}

uint64_t cache_sets(const cache *c)
{
    return c->sets;
}
