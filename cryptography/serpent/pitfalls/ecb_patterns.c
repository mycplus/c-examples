/* ecb_patterns.c - encrypting each 16-byte block independently (ECB) keeps
 * the pattern of the plaintext: equal input blocks give equal output blocks. */
#include <stdio.h>
#include <string.h>

#include "serpent.h"

int main(void)
{
    const char message[] = "PAY 100 DOLLARS PAY 100 DOLLARS PAY 999 DOLLARS ";
    uint8_t key[32];
    for (size_t i = 0; i < sizeof key; ++i)
        key[i] = (uint8_t)(0xa0 + i);
    serpent_key ks;
    serpent_set_key(&ks, key, sizeof key);

    for (size_t off = 0; off + 16 <= sizeof message - 1; off += 16) {
        uint8_t out[16];
        serpent_encrypt(&ks, (const uint8_t *)message + off, out);
        printf("\"%.16s\" -> ", message + off);
        for (size_t i = 0; i < 16; ++i)
            printf("%02x", out[i]);
        printf("\n");
    }
    serpent_wipe_key(&ks);
    return 0;
}
