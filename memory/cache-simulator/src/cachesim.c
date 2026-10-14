/* cachesim.c - runs a memory trace through a simulated cache.
 * Usage: cachesim <cache bytes> <direct|assoc|assoc:N> <lru|fifo> <block bytes> <trace file|->
 * Example: cachesim 32768 assoc:8 lru 64 trace.txt */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cache.h"
#include "trace.h"

static void usage(void)
{
    fprintf(stderr, "usage: cachesim <cache bytes> <direct|assoc|assoc:N> <lru|fifo> "
                    "<block bytes> <trace file|->\n");
}

/* Parses a whole decimal argument; rejects signs, spaces, junk and overflow. */
static int parse_count(const char *s, uint64_t *out)
{
    if (*s < '0' || *s > '9')
        return 0;
    char *end;
    errno = 0;
    const unsigned long long v = strtoull(s, &end, 10);
    if (errno == ERANGE || *end != '\0')
        return 0;
    *out = (uint64_t)v;
    return 1;
}

int main(int argc, char **argv)
{
    if (argc != 6) {
        usage();
        return 2;
    }
    cache_config cfg = {0};
    if (!parse_count(argv[1], &cfg.size) || !parse_count(argv[4], &cfg.block)) {
        usage();
        return 2;
    }
    if (strcmp(argv[3], "lru") == 0)
        cfg.policy = CACHE_LRU;
    else if (strcmp(argv[3], "fifo") == 0)
        cfg.policy = CACHE_FIFO;
    else {
        usage();
        return 2;
    }
    if (strcmp(argv[2], "direct") == 0)
        cfg.ways = 1;
    else if (strcmp(argv[2], "assoc") == 0)
        cfg.ways = cfg.block != 0 ? cfg.size / cfg.block : 0;     /* fully associative */
    else if (strncmp(argv[2], "assoc:", 6) != 0 || !parse_count(argv[2] + 6, &cfg.ways)) {
        usage();
        return 2;
    }
    const char *problem = cache_config_error(&cfg);
    if (problem != NULL) {
        fprintf(stderr, "cachesim: %s\n", problem);
        return 2;
    }

    FILE *in = strcmp(argv[5], "-") == 0 ? stdin : fopen(argv[5], "r");
    if (in == NULL) {
        fprintf(stderr, "cachesim: cannot open %s: %s\n", argv[5], strerror(errno));
        return 1;
    }
    cache *c = cache_create(&cfg);
    if (c == NULL) {
        fprintf(stderr, "cachesim: out of memory\n");
        return 1;
    }

    trace_reader r;
    trace_init(&r, in);
    trace_access a;
    int status;
    while ((status = trace_next(&r, &a)) == 1)
        cache_access(c, a.addr, a.size, a.is_write);
    if (in != stdin)
        fclose(in);
    if (status < 0) {
        fprintf(stderr, "cachesim: %s: line %lu is not a trace line\n", argv[5], r.line);
        cache_destroy(c);
        return 1;
    }

    const cache_stats *s = cache_get_stats(c);
    const uint64_t lookups = s->hits + s->misses;
    printf("Cache: %llu bytes, %llu-byte blocks, %llu-way, %llu set%s, %s\n",
           (unsigned long long)cfg.size, (unsigned long long)cfg.block,
           (unsigned long long)cfg.ways, (unsigned long long)cache_sets(c),
           cache_sets(c) == 1 ? "" : "s", cfg.policy == CACHE_LRU ? "LRU" : "FIFO");
    printf("Memory reads: %llu\n", (unsigned long long)s->mem_reads);
    printf("Memory writes: %llu\n", (unsigned long long)s->mem_writes);
    printf("Cache hits: %llu\n", (unsigned long long)s->hits);
    printf("Cache misses: %llu\n", (unsigned long long)s->misses);
    printf("Hit rate: %.2f%%\n", lookups ? 100.0 * (double)s->hits / (double)lookups : 0.0);
    cache_destroy(c);
    return 0;
}
