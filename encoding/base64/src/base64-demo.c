/* base64-demo.c - command line front end for the base64 library.
 *
 *   base64-demo encode "some text"
 *   base64-demo decode "c29tZSB0ZXh0"
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "base64.h"

static int do_encode(const char *text)
{
    const size_t len  = strlen(text);
    const size_t need = b64_encoded_size(len);

    char *out = malloc(need);
    if (out == NULL) {
        fputs("out of memory\n", stderr);
        return 1;
    }

    const size_t n = b64_encode((const unsigned char *)text, len, out, need);
    if (n == B64_ERROR) {
        fputs("encode failed\n", stderr);
        free(out);
        return 1;
    }

    printf("%s\n", out);
    free(out);
    return 0;
}

static int do_decode(const char *text)
{
    const size_t len  = strlen(text);
    const size_t need = b64_decoded_size(len);

    unsigned char *out = malloc(need ? need : 1);
    if (out == NULL) {
        fputs("out of memory\n", stderr);
        return 1;
    }

    const size_t n = b64_decode(text, len, out, need);
    if (n == B64_ERROR) {
        fputs("not valid base64\n", stderr);
        free(out);
        return 1;
    }

    fwrite(out, 1, n, stdout);   /* binary-safe: no %s, no NUL assumption */
    putchar('\n');
    free(out);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s encode|decode <text>\n", argv[0]);
        return 2;
    }
    if (strcmp(argv[1], "encode") == 0) return do_encode(argv[2]);
    if (strcmp(argv[1], "decode") == 0) return do_decode(argv[2]);

    fprintf(stderr, "unknown command \"%s\"\n", argv[1]);
    return 2;
}
