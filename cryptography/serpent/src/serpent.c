/* serpent.c - the Serpent block cipher in bitslice form, C11. */
#include "serpent.h"

#include <string.h>

/* The eight 4-bit S-boxes from the Serpent specification. */
static const uint8_t SBOX[8][16] = {
    { 3, 8, 15, 1, 10, 6, 5, 11, 14, 13, 4, 2, 7, 0, 9, 12 },
    { 15, 12, 2, 7, 9, 0, 5, 10, 1, 11, 14, 8, 6, 13, 3, 4 },
    { 8, 6, 7, 9, 3, 12, 10, 15, 13, 1, 14, 4, 0, 11, 5, 2 },
    { 0, 15, 11, 8, 12, 9, 6, 3, 13, 1, 2, 4, 10, 7, 5, 14 },
    { 1, 15, 8, 3, 12, 0, 11, 6, 2, 5, 4, 10, 9, 14, 7, 13 },
    { 15, 5, 2, 11, 4, 10, 9, 12, 0, 3, 14, 8, 13, 6, 7, 1 },
    { 7, 2, 12, 5, 8, 4, 6, 11, 14, 9, 1, 15, 13, 3, 10, 0 },
    { 1, 13, 15, 0, 14, 8, 2, 11, 7, 4, 12, 10, 9, 3, 5, 6 },
};

/* Inverse S-boxes, also from the specification. */
static const uint8_t SBOX_INV[8][16] = {
    { 13, 3, 11, 0, 10, 6, 5, 12, 1, 14, 4, 7, 15, 9, 8, 2 },
    { 5, 8, 2, 14, 15, 6, 12, 3, 11, 4, 7, 9, 1, 13, 10, 0 },
    { 12, 9, 15, 4, 11, 14, 1, 2, 0, 3, 6, 13, 5, 8, 10, 7 },
    { 0, 9, 10, 7, 11, 14, 6, 13, 3, 5, 12, 2, 4, 8, 15, 1 },
    { 5, 0, 8, 3, 10, 9, 7, 14, 2, 12, 11, 6, 4, 15, 13, 1 },
    { 8, 15, 2, 9, 4, 1, 13, 14, 11, 6, 5, 3, 7, 12, 10, 0 },
    { 15, 10, 1, 13, 5, 3, 6, 0, 4, 9, 14, 7, 2, 12, 8, 11 },
    { 3, 0, 6, 13, 9, 14, 15, 8, 5, 12, 11, 7, 10, 1, 4, 2 },
};

#define PHI 0x9e3779b9u /* fractional part of the golden ratio */

static uint32_t rotl(uint32_t x, unsigned n) { return (x << n) | (x >> (32 - n)); }
static uint32_t rotr(uint32_t x, unsigned n) { return (x >> n) | (x << (32 - n)); }

static uint32_t load_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 |
           (uint32_t)p[3] << 24;
}

static void store_le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

/* Bitslice S-box: bit j of x[0..3] forms a 4-bit input, x[0] the low bit.
 * All 32 bit positions go through the same 4-bit table. */
static void apply_sbox(const uint8_t box[16], uint32_t x[4])
{
    uint32_t y[4] = { 0, 0, 0, 0 };
    for (unsigned j = 0; j < 32; ++j) {
        unsigned in = ((x[0] >> j) & 1u) | ((x[1] >> j) & 1u) << 1 |
                      ((x[2] >> j) & 1u) << 2 | ((x[3] >> j) & 1u) << 3;
        unsigned out = box[in];
        for (unsigned b = 0; b < 4; ++b)
            y[b] |= (uint32_t)((out >> b) & 1u) << j;
    }
    memcpy(x, y, sizeof y);
}

/* The linear transformation that mixes the four words after each S-box. */
static void linear_transform(uint32_t x[4])
{
    x[0] = rotl(x[0], 13);
    x[2] = rotl(x[2], 3);
    x[1] ^= x[0] ^ x[2];
    x[3] ^= x[2] ^ (x[0] << 3);
    x[1] = rotl(x[1], 1);
    x[3] = rotl(x[3], 7);
    x[0] ^= x[1] ^ x[3];
    x[2] ^= x[3] ^ (x[1] << 7);
    x[0] = rotl(x[0], 5);
    x[2] = rotl(x[2], 22);
}

static void inverse_linear_transform(uint32_t x[4])
{
    x[2] = rotr(x[2], 22);
    x[0] = rotr(x[0], 5);
    x[2] ^= x[3] ^ (x[1] << 7);
    x[0] ^= x[1] ^ x[3];
    x[3] = rotr(x[3], 7);
    x[1] = rotr(x[1], 1);
    x[3] ^= x[2] ^ (x[0] << 3);
    x[1] ^= x[0] ^ x[2];
    x[2] = rotr(x[2], 3);
    x[0] = rotr(x[0], 13);
}

static void xor_key(uint32_t x[4], const uint32_t k[4])
{
    for (unsigned i = 0; i < 4; ++i)
        x[i] ^= k[i];
}

int serpent_set_key(serpent_key *ks, const uint8_t *key, size_t key_len)
{
    if (key_len == 0 || key_len > SERPENT_MAX_KEY_SIZE)
        return -1;

    /* Pad the key to 256 bits: one 1 bit after the key, then zeros. */
    uint8_t padded[SERPENT_MAX_KEY_SIZE] = { 0 };
    memcpy(padded, key, key_len);
    if (key_len < SERPENT_MAX_KEY_SIZE)
        padded[key_len] = 0x01;

    /* w[0..7] hold the padded key; w[8..139] are the 132 prekey words. */
    uint32_t w[140];
    for (unsigned i = 0; i < 8; ++i)
        w[i] = load_le32(padded + 4 * i);
    for (unsigned i = 8; i < 140; ++i)
        w[i] = rotl(w[i - 8] ^ w[i - 5] ^ w[i - 3] ^ w[i - 1] ^ PHI ^ (uint32_t)(i - 8), 11);

    /* Subkey i passes prekeys 4i..4i+3 through S-box (3 - i) mod 8. */
    for (unsigned i = 0; i < 33; ++i) {
        uint32_t *k = ks->subkey[i];
        memcpy(k, &w[8 + 4 * i], 4 * sizeof(uint32_t));
        apply_sbox(SBOX[(3 - i) & 7], k);
    }

    memset(padded, 0, sizeof padded);
    memset(w, 0, sizeof w);
    return 0;
}

void serpent_encrypt(const serpent_key *ks, const uint8_t in[SERPENT_BLOCK_SIZE],
                     uint8_t out[SERPENT_BLOCK_SIZE])
{
    uint32_t x[4];
    for (unsigned i = 0; i < 4; ++i)
        x[i] = load_le32(in + 4 * i);

    for (unsigned r = 0; r < 32; ++r) {
        xor_key(x, ks->subkey[r]);
        apply_sbox(SBOX[r & 7], x);
        if (r < 31)
            linear_transform(x);
        else
            xor_key(x, ks->subkey[32]);   /* the last round replaces LT with K32 */
    }

    for (unsigned i = 0; i < 4; ++i)
        store_le32(out + 4 * i, x[i]);
}

void serpent_decrypt(const serpent_key *ks, const uint8_t in[SERPENT_BLOCK_SIZE],
                     uint8_t out[SERPENT_BLOCK_SIZE])
{
    uint32_t x[4];
    for (unsigned i = 0; i < 4; ++i)
        x[i] = load_le32(in + 4 * i);

    for (int r = 31; r >= 0; --r) {
        if (r == 31)
            xor_key(x, ks->subkey[32]);
        else
            inverse_linear_transform(x);
        apply_sbox(SBOX_INV[r & 7], x);
        xor_key(x, ks->subkey[r]);
    }

    for (unsigned i = 0; i < 4; ++i)
        store_le32(out + 4 * i, x[i]);
}

void serpent_wipe_key(serpent_key *ks)
{
    /* volatile stops the compiler removing a store to memory it sees as dead */
    volatile uint8_t *p = (volatile uint8_t *)ks;
    for (size_t i = 0; i < sizeof *ks; ++i)
        p[i] = 0;
}
