/* digest.h - MD5, SHA-1 and SHA-256 with one streaming interface, plus HMAC-SHA-256.
 * For learning and testing. To protect real data, use a maintained library. */
#ifndef DIGEST_H
#define DIGEST_H

#include <stddef.h>
#include <stdint.h>

#define MD5_DIGEST_SIZE 16
#define SHA1_DIGEST_SIZE 20
#define SHA256_DIGEST_SIZE 32

/* All three hashes keep up to eight 32-bit state words and a 64-byte block. */
typedef struct {
    uint32_t h[8];       /* chaining value: 4 words (MD5), 5 (SHA-1) or 8 (SHA-256) */
    uint64_t bytes;      /* message bytes consumed so far */
    uint8_t buf[64];     /* bytes waiting for a full block */
} digest_ctx;

typedef digest_ctx md5_ctx;
typedef digest_ctx sha1_ctx;
typedef digest_ctx sha256_ctx;

void md5_init(md5_ctx *c);
void md5_update(md5_ctx *c, const void *data, size_t len);
void md5_final(md5_ctx *c, uint8_t out[MD5_DIGEST_SIZE]);

void sha1_init(sha1_ctx *c);
void sha1_update(sha1_ctx *c, const void *data, size_t len);
void sha1_final(sha1_ctx *c, uint8_t out[SHA1_DIGEST_SIZE]);

void sha256_init(sha256_ctx *c);
void sha256_update(sha256_ctx *c, const void *data, size_t len);
void sha256_final(sha256_ctx *c, uint8_t out[SHA256_DIGEST_SIZE]);

/* One-shot helpers. */
void md5(const void *data, size_t len, uint8_t out[MD5_DIGEST_SIZE]);
void sha1(const void *data, size_t len, uint8_t out[SHA1_DIGEST_SIZE]);
void sha256(const void *data, size_t len, uint8_t out[SHA256_DIGEST_SIZE]);

/* HMAC-SHA-256 (RFC 2104, RFC 4231). */
void hmac_sha256(const void *key, size_t key_len, const void *data, size_t len,
                 uint8_t out[SHA256_DIGEST_SIZE]);

/* Compares two digests in time that depends only on the length, not on where they differ. */
int digest_equal(const uint8_t *a, const uint8_t *b, size_t len);

/* Lower-case hex into out, which must hold 2 * len + 1 bytes. */
void digest_to_hex(const uint8_t *digest, size_t len, char *out);

/* Length-extension support: restart SHA-256 or MD5 from a published digest.
 * processed is the number of bytes the original hash consumed, padding included;
 * it must be a multiple of 64. */
void sha256_resume(sha256_ctx *c, const uint8_t digest[SHA256_DIGEST_SIZE], uint64_t processed);
void md5_resume(md5_ctx *c, const uint8_t digest[MD5_DIGEST_SIZE], uint64_t processed);

/* Writes the padding that a hash appends to a message of msg_len bytes (0x80, zeros,
 * the bit length) and returns its size. big_endian_length is 1 for SHA, 0 for MD5. */
size_t digest_padding(uint64_t msg_len, int big_endian_length, uint8_t out[72]);

#endif
