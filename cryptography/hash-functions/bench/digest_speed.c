/* digest_speed.c - throughput of this page's MD5, SHA-1 and SHA-256, and of OpenSSL's
 * when it is available. 64 MiB hashed in 64 KiB update calls; median of 5 runs.
 */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "digest.h"
#ifdef HAVE_OPENSSL
#include <openssl/evp.h>
#endif

#define TOTAL ((size_t)64 << 20)
#define CHUNK ((size_t)64 << 10)
#define RUNS 5

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static int compare_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static double median(double *v)
{
    qsort(v, RUNS, sizeof *v, compare_double);
    return v[RUNS / 2];
}

typedef void (*init_fn)(digest_ctx *);
typedef void (*update_fn)(digest_ctx *, const void *, size_t);
typedef void (*final_fn)(digest_ctx *, uint8_t *);

static double run_mine(init_fn init, update_fn update, final_fn fin, const uint8_t *buf, uint8_t *out)
{
    double t[RUNS];
    for (int r = 0; r < RUNS; ++r) {
        digest_ctx c;
        double start = now();
        init(&c);
        for (size_t i = 0; i < TOTAL; i += CHUNK)
            update(&c, buf, CHUNK);
        fin(&c, out);
        t[r] = now() - start;
    }
    return (double)(TOTAL >> 20) / median(t);
}

#ifdef HAVE_OPENSSL
static double run_openssl(const EVP_MD *type, const uint8_t *buf, uint8_t *out)
{
    double t[RUNS];
    for (int r = 0; r < RUNS; ++r) {
        unsigned int n = 0;
        EVP_MD_CTX *c = EVP_MD_CTX_new();
        double start = now();
        EVP_DigestInit_ex(c, type, NULL);
        for (size_t i = 0; i < TOTAL; i += CHUNK)
            EVP_DigestUpdate(c, buf, CHUNK);
        EVP_DigestFinal_ex(c, out, &n);
        t[r] = now() - start;
        EVP_MD_CTX_free(c);
    }
    return (double)(TOTAL >> 20) / median(t);
}
#endif

int main(void)
{
    uint8_t *buf = malloc(CHUNK);
    if (buf == NULL)
        return 1;
    for (size_t i = 0; i < CHUNK; ++i)
        buf[i] = (uint8_t)((i * 31 + 7) & 0xFF);

    static const struct {
        const char *name;
        init_fn init;
        update_fn update;
        final_fn fin;
        size_t size;
    } algos[3] = {
        { "MD5", md5_init, md5_update, md5_final, MD5_DIGEST_SIZE },
        { "SHA-1", sha1_init, sha1_update, sha1_final, SHA1_DIGEST_SIZE },
        { "SHA-256", sha256_init, sha256_update, sha256_final, SHA256_DIGEST_SIZE },
    };

    printf("%-9s %14s", "Hash", "this page");
#ifdef HAVE_OPENSSL
    printf(" %14s %8s", "OpenSSL", "ratio");
#endif
    printf("   digest of the buffer\n");
    for (int a = 0; a < 3; ++a) {
        uint8_t out[32];
        char h[65];
        double mine = run_mine(algos[a].init, algos[a].update, algos[a].fin, buf, out);
        digest_to_hex(out, algos[a].size, h);
        printf("%-9s %10.0f MB/s", algos[a].name, mine);
#ifdef HAVE_OPENSSL
        uint8_t theirs[64];
        const EVP_MD *type = a == 0 ? EVP_md5() : a == 1 ? EVP_sha1() : EVP_sha256();
        double lib = run_openssl(type, buf, theirs);
        printf(" %10.0f MB/s %7.1fx", lib, lib / mine);
        if (memcmp(out, theirs, algos[a].size) != 0)
            printf("  MISMATCH");
#endif
        printf("   %.16s...\n", h);
    }
    free(buf);
    return 0;
}
