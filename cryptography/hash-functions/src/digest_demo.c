/* digest_demo.c - MD5, SHA-1 and SHA-256 of three short messages.
 * Build: gcc -std=c11 -Wall -Wextra -pedantic digest_demo.c digest.c -o digest_demo
 */
#include <stdio.h>
#include <string.h>

#include "digest.h"

static void show(const char *label, const char *text)
{
    uint8_t d[SHA256_DIGEST_SIZE];
    char h[2 * SHA256_DIGEST_SIZE + 1];
    size_t n = strlen(text);

    printf("message: \"%s\" (%zu bytes)\n", label, n);
    md5(text, n, d);
    digest_to_hex(d, MD5_DIGEST_SIZE, h);
    printf("  MD5     %s\n", h);
    sha1(text, n, d);
    digest_to_hex(d, SHA1_DIGEST_SIZE, h);
    printf("  SHA-1   %s\n", h);
    sha256(text, n, d);
    digest_to_hex(d, SHA256_DIGEST_SIZE, h);
    printf("  SHA-256 %s\n", h);
}

int main(void)
{
    show("", "");
    show("abc", "abc");
    show("The quick brown fox jumps over the lazy dog", "The quick brown fox jumps over the lazy dog");
    return 0;
}
