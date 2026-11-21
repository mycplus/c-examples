/* strlen_on_binary.c - hashing binary data with strlen(). A zero byte ends the "length". */
#include <stdio.h>
#include <string.h>

#include "digest.h"

static void show(const char *label, const uint8_t *data, size_t len)
{
    uint8_t d[SHA256_DIGEST_SIZE];
    char h[2 * SHA256_DIGEST_SIZE + 1];
    sha256(data, len, d);
    digest_to_hex(d, SHA256_DIGEST_SIZE, h);
    printf("%-24s %2zu bytes  %s\n", label, len, h);
}

int main(void)
{
    const uint8_t record[] = { 'a', 0x00, 'b' };   /* three bytes, one of them zero */
    show("hashed with strlen():", record, strlen((const char *)record));
    show("hashed with sizeof:", record, sizeof record);
    return 0;
}
