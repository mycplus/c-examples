/* serpent.h - the Serpent block cipher (128-bit blocks, keys up to 256 bits)
 * in portable C11. Bytes are read as little-endian 32-bit words, the order
 * the NESSIE test vectors and libgcrypt use.
 *
 * For learning and testing. It has not been reviewed for side channels; use
 * a maintained cryptographic library for anything that protects real data.
 */
#ifndef MYCPLUS_SERPENT_H
#define MYCPLUS_SERPENT_H

#include <stddef.h>
#include <stdint.h>

#define SERPENT_BLOCK_SIZE 16
#define SERPENT_MAX_KEY_SIZE 32

typedef struct {
    uint32_t subkey[33][4]; /* K0..K32, one 128-bit subkey per round + 1 */
} serpent_key;

/* Expands a key of 1 to 32 bytes. Keys shorter than 32 bytes are padded as
 * the specification requires. Returns 0, or -1 if key_len is 0 or above 32. */
int serpent_set_key(serpent_key *ks, const uint8_t *key, size_t key_len);

void serpent_encrypt(const serpent_key *ks, const uint8_t in[SERPENT_BLOCK_SIZE],
                     uint8_t out[SERPENT_BLOCK_SIZE]);
void serpent_decrypt(const serpent_key *ks, const uint8_t in[SERPENT_BLOCK_SIZE],
                     uint8_t out[SERPENT_BLOCK_SIZE]);

/* Overwrites the expanded key so it does not linger in memory. */
void serpent_wipe_key(serpent_key *ks);

#endif
