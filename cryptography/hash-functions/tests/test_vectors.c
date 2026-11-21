/* test_vectors.c - checks MD5, SHA-1, SHA-256 and HMAC-SHA-256 against published and
 * independently generated answers, one-shot, in chunks and split at every offset.
 * Usage: test_vectors <vectors-directory>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "digest.h"

typedef void (*hash_fn)(const void *, size_t, uint8_t *);
typedef struct {
    const char *name;
    size_t size;
    hash_fn one_shot;
    void (*init)(digest_ctx *);
    void (*update)(digest_ctx *, const void *, size_t);
    void (*final)(digest_ctx *, uint8_t *);
} algo;

static void md5_one(const void *d, size_t n, uint8_t *o) { md5(d, n, o); }
static void sha1_one(const void *d, size_t n, uint8_t *o) { sha1(d, n, o); }
static void sha256_one(const void *d, size_t n, uint8_t *o) { sha256(d, n, o); }

static const algo ALGOS[3] = {
    { "md5", MD5_DIGEST_SIZE, md5_one, md5_init, md5_update, md5_final },
    { "sha1", SHA1_DIGEST_SIZE, sha1_one, sha1_init, sha1_update, sha1_final },
    { "sha256", SHA256_DIGEST_SIZE, sha256_one, sha256_init, sha256_update, sha256_final },
};

static const algo *find_algo(const char *name)
{
    for (int i = 0; i < 3; ++i)
        if (strcmp(ALGOS[i].name, name) == 0)
            return &ALGOS[i];
    return NULL;
}

static int failures = 0;

static void check(int ok, const char *what, const char *detail)
{
    if (!ok) {
        ++failures;
        printf("FAIL %s: %s\n", what, detail);
    }
}

static void hex(const uint8_t *d, size_t n, char *out) { digest_to_hex(d, n, out); }

/* Feeds the message in pieces of `chunk` bytes. */
static void hash_chunked(const algo *a, const uint8_t *msg, size_t len, size_t chunk, uint8_t *out)
{
    digest_ctx c;
    a->init(&c);
    for (size_t i = 0; i < len; i += chunk)
        a->update(&c, msg + i, len - i < chunk ? len - i : chunk);
    a->final(&c, out);
}

static uint8_t *pattern(size_t n)
{
    uint8_t *m = malloc(n ? n : 1);
    if (m == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(2);
    }
    for (size_t i = 0; i < n; ++i)
        m[i] = (uint8_t)((i * 31 + 7) & 0xFF);
    return m;
}

static FILE *open_vectors(const char *dir, const char *file)
{
    char path[1024];
    snprintf(path, sizeof path, "%s/%s", dir, file);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        fprintf(stderr, "cannot open %s\n", path);
        exit(2);
    }
    return f;
}

static int known_answers(const char *dir)
{
    FILE *f = open_vectors(dir, "known_answers.txt");
    char line[4096];
    int count = 0;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';
        /* Split on tabs by hand: strtok would skip the empty text of the empty message. */
        char *fields[4] = { line, NULL, NULL, NULL };
        int nf = 1;
        for (char *p = line; *p != '\0' && nf < 4; ++p) {
            if (*p == '\t') {
                *p = '\0';
                fields[nf++] = p + 1;
            }
        }
        if (nf != 4)
            continue;
        char *name = fields[0], *rep = fields[1], *text = fields[2], *want = fields[3];
        const algo *a = find_algo(name);
        if (a == NULL)
            continue;
        size_t n = (size_t)strtoul(rep, NULL, 10), tl = strlen(text);
        uint8_t *msg = malloc(n * tl + 1);
        if (msg == NULL)
            exit(2);
        for (size_t i = 0; i < n; ++i)
            memcpy(msg + i * tl, text, tl);
        uint8_t out[32];
        char h[65];
        a->one_shot(msg, n * tl, out);
        hex(out, a->size, h);
        check(strcmp(h, want) == 0, a->name, text);
        hash_chunked(a, msg, n * tl, 7, out);
        hex(out, a->size, h);
        check(strcmp(h, want) == 0, a->name, "7-byte chunks");
        free(msg);
        ++count;
    }
    fclose(f);
    return count;
}

static int lengths(const char *dir)
{
    FILE *f = open_vectors(dir, "lengths.txt");
    char line[512];
    int count = 0;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n')
            continue;
        char *len = strtok(line, "\t");
        char *want[3] = { strtok(NULL, "\t"), strtok(NULL, "\t"), strtok(NULL, "\t\r\n") };
        size_t n = (size_t)strtoul(len, NULL, 10);
        uint8_t *msg = pattern(n);
        for (int i = 0; i < 3; ++i) {
            uint8_t out[32];
            char h[65];
            ALGOS[i].one_shot(msg, n, out);
            hex(out, ALGOS[i].size, h);
            check(strcmp(h, want[i]) == 0, ALGOS[i].name, len);
            if (n <= 130) {      /* split the message at every offset */
                for (size_t cut = 0; cut <= n; ++cut) {
                    digest_ctx c;
                    ALGOS[i].init(&c);
                    ALGOS[i].update(&c, msg, cut);
                    ALGOS[i].update(&c, msg + cut, n - cut);
                    ALGOS[i].final(&c, out);
                    hex(out, ALGOS[i].size, h);
                    check(strcmp(h, want[i]) == 0, ALGOS[i].name, "split");
                }
            }
        }
        free(msg);
        ++count;
    }
    fclose(f);
    return count;
}

static size_t unhex(const char *s, uint8_t *out)
{
    size_t n = strlen(s) / 2;
    for (size_t i = 0; i < n; ++i) {
        unsigned v = 0;
        if (sscanf(s + 2 * i, "%2x", &v) != 1)
            exit(2);
        out[i] = (uint8_t)v;
    }
    return n;
}

static int hmac_vectors(const char *dir)
{
    FILE *f = open_vectors(dir, "hmac.txt");
    char line[1024];
    int count = 0;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n')
            continue;
        char *name = strtok(line, "\t");
        char *key = strtok(NULL, "\t");
        char *data = strtok(NULL, "\t");
        char *want = strtok(NULL, "\t\r\n");
        if (strcmp(name, "sha256") != 0)
            continue;            /* the C code implements HMAC-SHA-256 only */
        uint8_t k[256], d[256], out[32];
        char h[65];
        size_t kl = unhex(key, k), dl = unhex(data, d);
        hmac_sha256(k, kl, d, dl, out);
        hex(out, 32, h);
        check(strcmp(h, want) == 0, "hmac-sha256", key);
        ++count;
    }
    fclose(f);
    return count;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: test_vectors <vectors-directory>\n");
        return 2;
    }
    int ka = known_answers(argv[1]);
    int ln = lengths(argv[1]);
    int hm = hmac_vectors(argv[1]);

    /* The padding boundary: 55 bytes fit one block, 56 need two. */
    uint8_t pad[72];
    check(digest_padding(55, 1, pad) == 9, "padding", "55 bytes");
    check(digest_padding(56, 1, pad) == 72, "padding", "56 bytes");
    check(digest_padding(64, 1, pad) == 64, "padding", "64 bytes");

    /* Resuming from a digest: H(m || padding || s) must equal the hash continued from H(m). */
    {
        static const char m[] = "user=alice&amount=100", s[] = "&amount=1000000";
        uint8_t glue[72], full[256], d[32], resumed[32], direct[32];
        size_t ml = sizeof m - 1, sl = sizeof s - 1;
        for (int big = 0; big <= 1; ++big) {
            size_t gl = digest_padding(ml, big, glue);
            memcpy(full, m, ml);
            memcpy(full + ml, glue, gl);
            memcpy(full + ml + gl, s, sl);
            digest_ctx c;
            if (big) {
                sha256(m, ml, d);
                sha256_resume(&c, d, ml + gl);
                sha256_update(&c, s, sl);
                sha256_final(&c, resumed);
                sha256(full, ml + gl + sl, direct);
            } else {
                md5(m, ml, d);
                md5_resume(&c, d, ml + gl);
                md5_update(&c, s, sl);
                md5_final(&c, resumed);
                md5(full, ml + gl + sl, direct);
            }
            check(memcmp(resumed, direct, big ? 32 : 16) == 0, "resume", big ? "sha256" : "md5");
        }
    }

    uint8_t a[4] = { 1, 2, 3, 4 }, b[4] = { 1, 2, 3, 5 };
    check(digest_equal(a, a, 4) && !digest_equal(a, b, 4), "digest_equal", "");

    printf("known answers: %d\n", ka);
    printf("message lengths: %d (each also split at every offset up to 130 bytes)\n", ln);
    printf("HMAC-SHA-256 vectors: %d\n", hm);
    printf("failures: %d\n", failures);
    return failures == 0 ? 0 : 1;
}
