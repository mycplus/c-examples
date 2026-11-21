/* hex_sign_extension.c - printing a digest through a signed char buffer.
 * On x86, plain `char` is signed, and printf("%02x") of a negative char prints ffffff90.
 */
#include <stdio.h>

#include "digest.h"

int main(void)
{
    uint8_t digest[MD5_DIGEST_SIZE];
    md5("abc", 3, digest);

    signed char wrong[MD5_DIGEST_SIZE];       /* what a plain char buffer is on x86 and MSVC */
    for (size_t i = 0; i < sizeof digest; ++i)
        wrong[i] = (signed char)digest[i];

    printf("printed through signed char:\n  ");
    for (size_t i = 0; i < 4; ++i)
        printf("%02x ", wrong[i]);
    printf("...\nprinted through uint8_t:\n  ");
    for (size_t i = 0; i < 4; ++i)
        printf("%02x ", digest[i]);
    printf("...\n");
    return 0;
}
