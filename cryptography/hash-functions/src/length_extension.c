/* length_extension.c - forging a signature for H(secret || message) without the secret,
 * for SHA-256 and MD5, and why HMAC-SHA-256 is not affected.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic length_extension.c digest.c -o length_extension
 */
#include <stdio.h>
#include <string.h>

#include "digest.h"

static const char SECRET[] = "k3y-of-16-bytes!";            /* known only to the server */
#define SECRET_LEN (sizeof SECRET - 1)

/* One hash, seen through the same four operations. */
typedef struct {
    const char *name;
    size_t size;
    int big_endian_length;
    void (*init)(digest_ctx *);
    void (*update)(digest_ctx *, const void *, size_t);
    void (*final)(digest_ctx *, uint8_t *);
    void (*resume)(digest_ctx *, const uint8_t *, uint64_t);
} scheme;

static void sha256_resume_any(digest_ctx *c, const uint8_t *d, uint64_t n) { sha256_resume(c, d, n); }
static void md5_resume_any(digest_ctx *c, const uint8_t *d, uint64_t n) { md5_resume(c, d, n); }

static const scheme SHA256_SCHEME = { "SHA-256", SHA256_DIGEST_SIZE, 1, sha256_init, sha256_update,
                                      sha256_final, sha256_resume_any };
static const scheme MD5_SCHEME = { "MD5", MD5_DIGEST_SIZE, 0, md5_init, md5_update, md5_final,
                                   md5_resume_any };

/* The server's signature: H(secret || message). */
static void sign(const scheme *s, const uint8_t *msg, size_t len, uint8_t *tag)
{
    digest_ctx c;
    s->init(&c);
    s->update(&c, SECRET, SECRET_LEN);
    s->update(&c, msg, len);
    s->final(&c, tag);
}

static void print_escaped(const uint8_t *p, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        if (p[i] >= 0x20 && p[i] < 0x7f)
            putchar(p[i]);
        else
            printf("\\x%02x", p[i]);
    }
}

static void attack(const scheme *s, const char *message, const char *suffix)
{
    uint8_t tag[SHA256_DIGEST_SIZE], forged_tag[SHA256_DIGEST_SIZE];
    char hex_tag[2 * SHA256_DIGEST_SIZE + 1];
    size_t msg_len = strlen(message), suffix_len = strlen(suffix);

    sign(s, (const uint8_t *)message, msg_len, tag);
    digest_to_hex(tag, s->size, hex_tag);
    printf("%s: the server signed \"%s\" and published the tag\n  %s\n", s->name, message, hex_tag);

    /* The attacker knows the message, the tag and the algorithm, but not the secret
     * or even its length, so tries every plausible length. */
    for (size_t guess = 1; guess <= 32; ++guess) {
        uint8_t glue[72], forged_msg[256];
        size_t glue_len = digest_padding(guess + msg_len, s->big_endian_length, glue);

        digest_ctx c;
        s->resume(&c, tag, guess + msg_len + glue_len);   /* continue from the published tag */
        s->update(&c, suffix, suffix_len);
        s->final(&c, forged_tag);

        memcpy(forged_msg, message, msg_len);              /* message || padding || suffix */
        memcpy(forged_msg + msg_len, glue, glue_len);
        memcpy(forged_msg + msg_len + glue_len, suffix, suffix_len);
        size_t forged_len = msg_len + glue_len + suffix_len;

        uint8_t expected[SHA256_DIGEST_SIZE];
        sign(s, forged_msg, forged_len, expected);         /* what the server computes */
        if (digest_equal(expected, forged_tag, s->size)) {
            digest_to_hex(forged_tag, s->size, hex_tag);
            printf("  forged message: ");
            print_escaped(forged_msg, forged_len);
            printf("\n  forged tag:     %s\n", hex_tag);
            printf("  the server accepts it (the attacker's guess of the secret length: %zu)\n", guess);
            return;
        }
    }
    printf("  no forgery\n");
}

static void hmac_defence(const char *message, const char *suffix)
{
    uint8_t tag[SHA256_DIGEST_SIZE], forged_tag[SHA256_DIGEST_SIZE], expected[SHA256_DIGEST_SIZE];
    char hex_tag[2 * SHA256_DIGEST_SIZE + 1];
    size_t msg_len = strlen(message), suffix_len = strlen(suffix);
    int accepted = 0;

    hmac_sha256(SECRET, SECRET_LEN, message, msg_len, tag);
    digest_to_hex(tag, SHA256_DIGEST_SIZE, hex_tag);
    printf("HMAC-SHA-256: the server signed \"%s\" and published the tag\n  %s\n", message, hex_tag);

    /* The same attack: resume SHA-256 from the HMAC tag, for every guessed length. */
    for (size_t guess = 1; guess <= 64 && !accepted; ++guess) {
        uint8_t glue[72], forged_msg[256];
        size_t glue_len = digest_padding(guess + msg_len, 1, glue);
        sha256_ctx c;
        sha256_resume(&c, tag, guess + msg_len + glue_len);
        sha256_update(&c, suffix, suffix_len);
        sha256_final(&c, forged_tag);

        memcpy(forged_msg, message, msg_len);
        memcpy(forged_msg + msg_len, glue, glue_len);
        memcpy(forged_msg + msg_len + glue_len, suffix, suffix_len);
        hmac_sha256(SECRET, SECRET_LEN, forged_msg, msg_len + glue_len + suffix_len, expected);
        accepted = digest_equal(expected, forged_tag, SHA256_DIGEST_SIZE);
    }
    printf("  the same attack, guessing every secret length from 1 to 64: %s\n",
           accepted ? "ACCEPTED" : "rejected every time");
}

int main(void)
{
    const char *message = "user=alice&amount=100";
    const char *suffix = "&amount=1000000";

    attack(&SHA256_SCHEME, message, suffix);
    printf("\n");
    attack(&MD5_SCHEME, message, suffix);
    printf("\n");
    hmac_defence(message, suffix);
    return 0;
}
