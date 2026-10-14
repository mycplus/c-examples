/* test_against_libgcrypt.c - compares this implementation with libgcrypt's
 * Serpent on random keys and blocks, for 128-, 192- and 256-bit keys.
 * Built only when libgcrypt is found. */
#include <gcrypt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "serpent.h"

static uint64_t state = UINT64_C(0x9E3779B97F4A7C15);
static uint8_t next_byte(void)
{
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return (uint8_t)(state >> 24);
}

int main(void)
{
    if (!gcry_check_version(GCRYPT_VERSION)) {
        fputs("libgcrypt version mismatch\n", stderr);
        return EXIT_FAILURE;
    }
    gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

    const struct { int algo; size_t key_len; } cases[] = {
        { GCRY_CIPHER_SERPENT128, 16 },
        { GCRY_CIPHER_SERPENT192, 24 },
        { GCRY_CIPHER_SERPENT256, 32 },
    };
    unsigned compared = 0, mismatches = 0;
    for (size_t c = 0; c < sizeof cases / sizeof cases[0]; ++c) {
        for (int trial = 0; trial < 3000; ++trial) {
            uint8_t key[32], block[16], mine[16], theirs[16], back[16];
            for (size_t i = 0; i < cases[c].key_len; ++i)
                key[i] = next_byte();
            for (size_t i = 0; i < 16; ++i)
                block[i] = next_byte();

            serpent_key ks;
            serpent_set_key(&ks, key, cases[c].key_len);
            serpent_encrypt(&ks, block, mine);
            serpent_decrypt(&ks, mine, back);

            gcry_cipher_hd_t h;
            if (gcry_cipher_open(&h, cases[c].algo, GCRY_CIPHER_MODE_ECB, 0) ||
                gcry_cipher_setkey(h, key, cases[c].key_len) ||
                gcry_cipher_encrypt(h, theirs, 16, block, 16)) {
                fputs("libgcrypt call failed\n", stderr);
                return EXIT_FAILURE;
            }
            gcry_cipher_close(h);

            ++compared;
            if (memcmp(mine, theirs, 16) != 0 || memcmp(back, block, 16) != 0)
                ++mismatches;
        }
    }
    printf("compared %u blocks with libgcrypt %s: %u mismatches\n", compared,
           gcry_check_version(NULL), mismatches);
    return mismatches == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
