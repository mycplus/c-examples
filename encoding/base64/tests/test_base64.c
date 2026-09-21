/* Test vectors from RFC 4648 section 10, plus round-trip and
   input-validation checks. Exits non-zero on the first failure. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "base64.h"

static int failures = 0;

static void check(int ok, const char *what)
{
    printf("  %-58s %s\n", what, ok ? "ok" : "FAIL");
    if (!ok) failures++;
}

static void vector(const char *plain, const char *expected)
{
    char enc[128];
    unsigned char dec[128];

    const size_t n = b64_encode((const unsigned char *)plain,
                                strlen(plain), enc, sizeof enc);
    char label[96];

    snprintf(label, sizeof label, "encode(\"%s\") == \"%s\"", plain, expected);
    check(n != B64_ERROR && strcmp(enc, expected) == 0, label);

    const size_t m = b64_decode(expected, strlen(expected), dec, sizeof dec);
    snprintf(label, sizeof label, "decode(\"%s\") == \"%s\"", expected, plain);
    check(m != B64_ERROR && m == strlen(plain)
          && memcmp(dec, plain, m) == 0, label);
}

static void reject(const char *bad, const char *why)
{
    unsigned char dec[128];
    check(b64_decode(bad, strlen(bad), dec, sizeof dec) == B64_ERROR, why);
}

int main(void)
{
    puts("RFC 4648 test vectors");
    vector("",       "");
    vector("f",      "Zg==");
    vector("fo",     "Zm8=");
    vector("foo",    "Zm9v");
    vector("foob",   "Zm9vYg==");
    vector("fooba",  "Zm9vYmE=");
    vector("foobar", "Zm9vYmFy");

    puts("\nRound trip over every byte value");
    unsigned char all[256], back[256];
    char enc[512];
    for (int i = 0; i < 256; ++i) all[i] = (unsigned char)i;
    const size_t n = b64_encode(all, sizeof all, enc, sizeof enc);
    const size_t m = b64_decode(enc, n, back, sizeof back);
    check(n != B64_ERROR && m == sizeof all
          && memcmp(all, back, sizeof all) == 0,
          "all 256 byte values survive a round trip");

    puts("\nInvalid input is rejected");
    reject("A",          "length not a multiple of four");
    reject("!!!!",       "characters outside the alphabet");
    reject("\xff\xff\xff\xff", "bytes >= 0x80");
    reject("A=AA",       "padding before the end of the last group");
    reject("=AAA",       "padding in the first position");
    reject("Zm9vYg=A",   "data after padding");
    reject("Zh==",       "non-canonical padding bits (one byte)");
    reject("Zm9=",       "non-canonical padding bits (two bytes)");

    puts("\nBuffer sizing");
    char small[4];
    check(b64_encode((const unsigned char *)"foobar", 6, small, sizeof small)
          == B64_ERROR, "encode refuses a buffer that is too small");
    unsigned char tiny[1];
    check(b64_decode("Zm9vYmFy", 8, tiny, sizeof tiny)
          == B64_ERROR, "decode refuses a buffer that is too small");
    check(b64_encoded_size(0) == 1, "encoded_size(0) leaves room for the NUL");
    check(b64_encoded_size(SIZE_MAX) == B64_ERROR,
          "encoded_size reports overflow instead of wrapping");
    check(b64_encoded_size(SIZE_MAX / 2) != B64_ERROR,
          "encoded_size still answers for large in-range sizes");
    check(b64_encoded_size(SIZE_MAX / 4 * 3 + 3) == B64_ERROR,
          "encoded_size reports overflow just past three quarters");

    printf("\n%s\n", failures ? "FAILURES" : "all checks passed");
    return failures != 0;
}
