/* trailing_newline.c - "abc" and "abc\n" are different messages. fgets() keeps the newline,
 * and so does `echo abc | sha256sum`. */
#include <stdio.h>
#include <string.h>

#include "digest.h"

static void show(const char *label, const char *text)
{
    uint8_t d[SHA256_DIGEST_SIZE];
    char h[2 * SHA256_DIGEST_SIZE + 1];
    sha256(text, strlen(text), d);
    digest_to_hex(d, SHA256_DIGEST_SIZE, h);
    printf("%-18s %s\n", label, h);
}

int main(void)
{
    char line[64];
    snprintf(line, sizeof line, "%s", "abc\n");   /* what fgets() returns for the input line abc */
    show("\"abc\":", "abc");
    show("\"abc\\n\" (fgets):", line);
    line[strcspn(line, "\n")] = '\0';             /* strip the newline before hashing */
    show("after stripping:", line);
    return 0;
}
