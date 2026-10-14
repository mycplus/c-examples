/* serpent_demo.c - encrypts and decrypts one block with a 256-bit key, then
 * checks two published test vectors: one in NESSIE byte order, and one from
 * the Serpent AES submission, which writes every value byte-reversed.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic serpent_demo.c serpent.c -o serpent_demo
 */
#include <stdio.h>
#include <string.h>

#include "serpent.h"

static void print_hex(const char *label, const uint8_t *p, size_t n)
{
    printf("%-11s", label);
    for (size_t i = 0; i < n; ++i)
        printf("%02x", p[i]);
    printf("\n");
}

static void reverse(uint8_t *p, size_t n)
{
    for (size_t i = 0; i < n / 2; ++i) {
        uint8_t t = p[i];
        p[i] = p[n - 1 - i];
        p[n - 1 - i] = t;
    }
}

int main(void)
{
    uint8_t key[32];
    for (size_t i = 0; i < sizeof key; ++i)
        key[i] = (uint8_t)i;                         /* 00 01 02 ... 1f */
    static const char text[] = "Serpent: 128-bit";   /* 16 characters + NUL */
    uint8_t plain[16];
    memcpy(plain, text, sizeof plain);                /* the block is the 16 characters */

    serpent_key ks;
    if (serpent_set_key(&ks, key, sizeof key) != 0)
        return 1;
    uint8_t cipher[16], back[16];
    serpent_encrypt(&ks, plain, cipher);
    serpent_decrypt(&ks, cipher, back);
    serpent_wipe_key(&ks);

    print_hex("key", key, sizeof key);
    print_hex("plaintext", plain, sizeof plain);
    print_hex("ciphertext", cipher, sizeof cipher);
    printf("decrypts back: %s\n", memcmp(back, plain, 16) == 0 ? "yes" : "no");

    /* NESSIE: 128-bit key 80 00 .. 00, zero block -> 264e5481... */
    uint8_t k128[16] = { 0x80 }, zero[16] = { 0 }, out[16];
    const uint8_t nessie[16] = { 0x26, 0x4e, 0x54, 0x81, 0xef, 0xf4, 0x2a, 0x46,
                                 0x06, 0xab, 0xda, 0x06, 0xc0, 0xbf, 0xda, 0x3d };
    serpent_set_key(&ks, k128, sizeof k128);
    serpent_encrypt(&ks, zero, out);
    printf("NESSIE vector:          %s\n", memcmp(out, nessie, 16) == 0 ? "match" : "MISMATCH");

    /* AES submission: 256-bit zero key, zero block -> 89104945... once the
     * output bytes are reversed (the key and block are all zero either way). */
    const uint8_t submission[16] = { 0x89, 0x10, 0x49, 0x45, 0x04, 0x18, 0x19, 0x50,
                                     0xf9, 0x8d, 0xd9, 0x98, 0xa8, 0x2b, 0x67, 0x49 };
    uint8_t k256[32] = { 0 };
    serpent_set_key(&ks, k256, sizeof k256);
    serpent_encrypt(&ks, zero, out);
    reverse(out, sizeof out);
    printf("AES submission vector:  %s\n", memcmp(out, submission, 16) == 0 ? "match" : "MISMATCH");
    serpent_wipe_key(&ks);
    return 0;
}
