/* Tests for cache.c and trace.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cache.h"
#include "trace.h"

static int failures = 0;
#define CHECK(cond)                                                              \
    if (!(cond)) {                                                               \
        fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
        failures++;                                                              \
    }

static cache *make(uint64_t size, uint64_t block, uint64_t ways, cache_policy p)
{
    const cache_config cfg = {size, block, ways, p};
    return cache_create(&cfg);
}

static void test_config(void)
{
    const cache_config good[] = {
        {32768, 64, 8, CACHE_LRU}, {49152, 64, 12, CACHE_LRU},  /* 12-way, 64 sets */
        {64, 64, 1, CACHE_FIFO},   {3, 1, 3, CACHE_FIFO}};
    for (size_t i = 0; i < sizeof good / sizeof good[0]; i++)
        CHECK(cache_config_error(&good[i]) == NULL);

    const cache_config bad[] = {
        {0, 64, 1, CACHE_LRU},     {32768, 0, 1, CACHE_LRU},  {32768, 48, 1, CACHE_LRU},
        {32768, 64, 0, CACHE_LRU}, {32768, 64, 3, CACHE_LRU}, /* 512 blocks / 3 */
        {32, 64, 1, CACHE_LRU},    {98304, 64, 8, CACHE_LRU}, /* 192 sets */
        {32768, 64, 1024, CACHE_LRU}};
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        CHECK(cache_config_error(&bad[i]) != NULL);
        CHECK(cache_create(&bad[i]) == NULL);
    }
}

/* The example from the original article: two lines, accesses A, B, A, C.
 * LRU evicts B (used less recently than A); FIFO evicts A (loaded first). */
static void test_lru_versus_fifo(void)
{
    const uint64_t A = 0x000, B = 0x100, C = 0x200;
    cache *lru = make(512, 256, 2, CACHE_LRU), *fifo = make(512, 256, 2, CACHE_FIFO);
    const uint64_t seq[] = {A, B, A, C};
    for (int i = 0; i < 4; i++) {
        cache_access(lru, seq[i], 1, false);
        cache_access(fifo, seq[i], 1, false);
    }
    CHECK(cache_access(lru, A, 1, false) == true);    /* A survived */
    CHECK(cache_access(lru, B, 1, false) == false);   /* B was evicted */
    CHECK(cache_access(fifo, B, 1, false) == true);   /* B survived */
    CHECK(cache_access(fifo, A, 1, false) == false);  /* A was evicted */
    cache_destroy(lru);
    cache_destroy(fifo);
}

static unsigned long long belady(cache_policy p, uint64_t lines)
{
    static const uint64_t refs[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    cache *c = make(lines, 1, lines, p);
    for (size_t i = 0; i < 12; i++)
        cache_access(c, refs[i], 1, false);
    const unsigned long long m = (unsigned long long)cache_get_stats(c)->misses;
    cache_destroy(c);
    return m;
}

static void test_belady(void)
{
    CHECK(belady(CACHE_FIFO, 3) == 9 && belady(CACHE_FIFO, 4) == 10);   /* the anomaly */
    CHECK(belady(CACHE_LRU, 3) == 10 && belady(CACHE_LRU, 4) == 8);
}

static void test_addresses_and_blocks(void)
{
    cache *c = make(32768, 64, 8, CACHE_LRU);
    /* 48-bit addresses that differ only above bit 31 are different blocks. */
    CHECK(cache_access(c, 0x7ffd00000100ULL, 1, false) == false);
    CHECK(cache_access(c, 0x7ffe00000100ULL, 1, false) == false);
    CHECK(cache_access(c, 0x7ffd00000100ULL, 1, false) == true);
    /* An 8-byte access at offset 60 of a 64-byte block touches two blocks. */
    CHECK(cache_access(c, 0x103c, 8, false) == false);
    CHECK(cache_get_stats(c)->misses == 4);           /* 2 above + 2 blocks here */
    CHECK(cache_access(c, 0x1040, 1, false) == true);
    /* Write-through: every write goes to memory, hit or miss. */
    cache_access(c, 0x1040, 4, true);
    cache_access(c, 0x9000, 4, true);
    const cache_stats *s = cache_get_stats(c);
    CHECK(s->writes == 2 && s->mem_writes == 2 && s->mem_reads == s->misses);
    cache_destroy(c);

    /* Direct-mapped: two blocks 4 KiB apart share a set and evict each other. */
    cache *d = make(4096, 64, 1, CACHE_LRU);
    for (int i = 0; i < 4; i++) {
        cache_access(d, 0x0000, 1, false);
        cache_access(d, 0x1000, 1, false);
    }
    CHECK(cache_get_stats(d)->hits == 0 && cache_get_stats(d)->evictions == 7);
    cache_destroy(d);
}

static int parse_all(const char *text, trace_access *out, int max, unsigned long *bad_line)
{
    FILE *f = fopen("test_cache_trace.tmp", "w+");     /* tmpfile() is unreliable on Windows */
    if (f == NULL)
        return -2;
    fputs(text, f);
    rewind(f);
    trace_reader r;
    trace_init(&r, f);
    int n = 0, st;
    while ((st = trace_next(&r, &out[n < max ? n : max - 1])) == 1)
        n++;
    *bad_line = r.line;
    fclose(f);
    remove("test_cache_trace.tmp");
    return st < 0 ? -1 : n;
}

static void test_trace_parser(void)
{
    trace_access a[16];
    unsigned long line;
    CHECK(parse_all("0x400a10: R 0x1000\nW 0x7ffd5a2c\n#eof\nR 0x1\n", a, 16, &line) == 2);
    CHECK(a[0].addr == 0x1000 && !a[0].is_write && a[1].addr == 0x7ffd5a2c && a[1].is_write);

    const char *lackey = "==9== Lackey\nI  04016b20,3\n L 1ffefffd40,8\n S 1ffefffd48,4\n"
                         " M 0401d1f0,4\n\n# comment\n";
    CHECK(parse_all(lackey, a, 16, &line) == 4);         /* I skipped, M is two */
    CHECK(a[0].addr == 0x1ffefffd40ULL && a[0].size == 8 && !a[0].is_write);
    CHECK(a[1].is_write && a[1].size == 4);
    CHECK(a[2].addr == 0x0401d1f0 && !a[2].is_write && a[3].addr == 0x0401d1f0 && a[3].is_write);

    const char *bad[] = {"R\n", "X 0x10\n", "R 0x\n", "R 0x10 extra\n", " L 10\n",
                         " L 10,0\n", " L 10,-4\n", "R 0x10000000000000000\n", "R -16\n"};
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++)
        CHECK(parse_all(bad[i], a, 16, &line) == -1);
    CHECK(parse_all("R 0x1\nR 0x2\nW zz\n", a, 16, &line) == -1 && line == 3);

    char longline[400];
    memset(longline, 'f', sizeof longline);
    memcpy(longline, "R 0x", 4);
    longline[sizeof longline - 2] = '\n';
    longline[sizeof longline - 1] = '\0';
    CHECK(parse_all(longline, a, 16, &line) == -1);      /* too long, not truncated */
}

int main(void)
{
    test_config();
    test_lru_versus_fifo();
    test_belady();
    test_addresses_and_blocks();
    test_trace_parser();
    if (failures != 0) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    printf("cache: all tests passed\n");
    return EXIT_SUCCESS;
}
