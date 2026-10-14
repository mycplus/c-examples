/* serpent_speed.c - encryption speed in ECB mode over a buffer of N MiB
 * (default 16), median of 5 runs, for this implementation and, when built
 * with libgcrypt, for libgcrypt's Serpent. Every language's benchmark fills
 * the buffer the same way and prints the XOR of all ciphertext blocks, so the
 * results can be compared. POSIX clock; not run by the tests.
 * Usage: serpent_speed [MiB] */
#define _POSIX_C_SOURCE 199309L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "serpent.h"
#ifdef HAVE_LIBGCRYPT
#include <gcrypt.h>
#endif

#define RUNS 5

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static int cmp(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static void print_fold(const uint8_t *buf, size_t bytes)
{
    uint8_t fold[16] = { 0 };
    for (size_t i = 0; i < bytes; ++i)
        fold[i % 16] ^= buf[i];
    printf("  xor of blocks ");
    for (int i = 0; i < 16; ++i)
        printf("%02x", fold[i]);
    printf("\n");
}

int main(int argc, char **argv)
{
    long mib = argc > 1 ? strtol(argv[1], NULL, 10) : 16;
    if (mib < 1 || mib > 1024)
        return EXIT_FAILURE;
    size_t bytes = (size_t)mib << 20;
    uint8_t *in = malloc(bytes), *mine = malloc(bytes), *theirs = malloc(bytes);
    if (in == NULL || mine == NULL || theirs == NULL)
        return EXIT_FAILURE;
    for (size_t i = 0; i < bytes; ++i)
        in[i] = (uint8_t)((uint32_t)(i * 2654435761u) >> 24);
    uint8_t key[32];
    for (int i = 0; i < 32; ++i)
        key[i] = (uint8_t)i;

    serpent_key ks;
    serpent_set_key(&ks, key, 32);
    double t[RUNS];
    for (int r = 0; r < RUNS; ++r) {
        double s = now();
        for (size_t off = 0; off < bytes; off += 16)
            serpent_encrypt(&ks, in + off, mine + off);
        t[r] = now() - s;
    }
    qsort(t, RUNS, sizeof t[0], cmp);
    printf("C:           %8.2f MB/s (median of %d, %ld MiB)", (double)bytes / t[RUNS / 2] / 1e6,
           RUNS, mib);
    print_fold(mine, bytes);

#ifdef HAVE_LIBGCRYPT
    gcry_check_version(NULL);
    gcry_cipher_hd_t h;
    gcry_cipher_open(&h, GCRY_CIPHER_SERPENT256, GCRY_CIPHER_MODE_ECB, 0);
    gcry_cipher_setkey(h, key, 32);
    for (int r = 0; r < RUNS; ++r) {
        double s = now();
        gcry_cipher_encrypt(h, theirs, bytes, in, bytes);
        t[r] = now() - s;
    }
    gcry_cipher_close(h);
    qsort(t, RUNS, sizeof t[0], cmp);
    printf("libgcrypt %s: %8.2f MB/s, same ciphertext: %s\n", gcry_check_version(NULL),
           (double)bytes / t[RUNS / 2] / 1e6, memcmp(mine, theirs, bytes) == 0 ? "yes" : "no");
#endif
    serpent_wipe_key(&ks);
    free(in);
    free(mine);
    free(theirs);
    return 0;
}
