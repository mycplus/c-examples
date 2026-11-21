/* test_against_openssl.c - 5,000 random messages and HMAC keys, hashed here and by OpenSSL. */
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "digest.h"

static uint64_t rng_state = 0x9e3779b97f4a7c15ULL;
static uint32_t next_random(void)                 /* xorshift64*: fixed seed, same run every time */
{
    rng_state ^= rng_state >> 12;
    rng_state ^= rng_state << 25;
    rng_state ^= rng_state >> 27;
    return (uint32_t)((rng_state * 0x2545F4914F6CDD1DULL) >> 32);
}

static int openssl_digest(const EVP_MD *type, const uint8_t *msg, size_t len, uint8_t *out)
{
    unsigned int n = 0;
    return EVP_Digest(msg, len, out, &n, type, NULL);
}

int main(void)
{
    static uint8_t msg[4200], key[200];
    int checked = 0, bad = 0;
    for (int i = 0; i < 5000; ++i) {
        size_t len = next_random() % 4100, key_len = 1 + next_random() % 150;
        for (size_t j = 0; j < len; ++j)
            msg[j] = (uint8_t)next_random();
        for (size_t j = 0; j < key_len; ++j)
            key[j] = (uint8_t)next_random();

        uint8_t mine[32], theirs[32];
        unsigned int hl = 0;
        md5(msg, len, mine);
        bad += !(openssl_digest(EVP_md5(), msg, len, theirs) && memcmp(mine, theirs, 16) == 0);
        sha1(msg, len, mine);
        bad += !(openssl_digest(EVP_sha1(), msg, len, theirs) && memcmp(mine, theirs, 20) == 0);
        sha256(msg, len, mine);
        bad += !(openssl_digest(EVP_sha256(), msg, len, theirs) && memcmp(mine, theirs, 32) == 0);
        hmac_sha256(key, key_len, msg, len, mine);
        bad += !(HMAC(EVP_sha256(), key, (int)key_len, msg, len, theirs, &hl) != NULL && hl == 32 &&
                 memcmp(mine, theirs, 32) == 0);
        checked += 4;
    }
    printf("compared %d digests and MACs with OpenSSL %s: %d mismatches\n", checked,
           OpenSSL_version(OPENSSL_VERSION_STRING), bad);
    return bad == 0 ? 0 : 1;
}
