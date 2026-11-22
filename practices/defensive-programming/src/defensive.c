/* defensive.c - see defensive.h */
#include "defensive.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const char *dp_strerror(dp_status status)
{
    switch (status) {
    case DP_OK:               return "ok";
    case DP_ERR_NULL:         return "missing argument";
    case DP_ERR_EMPTY:        return "empty input";
    case DP_ERR_NOT_A_NUMBER: return "not a number";
    case DP_ERR_TRAILING:     return "extra characters after the number";
    case DP_ERR_RANGE:        return "number out of range";
    case DP_ERR_TOO_LONG:     return "input too long";
    case DP_ERR_EOF:          return "end of input";
    case DP_ERR_OVERFLOW:     return "size overflow";
    case DP_ERR_NO_MEMORY:    return "out of memory";
    case DP_ERR_IO:           return "input/output error";
    }
    return "unknown error"; /* a value outside the enum */
}

dp_status dp_parse_long(const char *text, long min, long max, long *out)
{
    if (text == NULL || out == NULL) {
        return DP_ERR_NULL;
    }
    while (isspace((unsigned char)*text)) {
        text++;
    }
    if (*text == '\0') {
        return DP_ERR_EMPTY;
    }

    char *end = NULL;
    errno = 0;
    const long value = strtol(text, &end, 10);
    if (end == text) {
        return DP_ERR_NOT_A_NUMBER;
    }
    if (errno == ERANGE || value < min || value > max) {
        return DP_ERR_RANGE;
    }
    if (*end != '\0') {
        return DP_ERR_TRAILING;
    }
    *out = value;
    return DP_OK;
}

dp_status dp_read_line(FILE *in, char *buf, size_t size)
{
    if (in == NULL || buf == NULL) {
        return DP_ERR_NULL;
    }
    if (size < 2 || size > INT_MAX) {
        return DP_ERR_RANGE; /* fgets takes an int and needs room for one char */
    }
    if (fgets(buf, (int)size, in) == NULL) {
        buf[0] = '\0';
        return ferror(in) ? DP_ERR_IO : DP_ERR_EOF;
    }
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[--len] = '\0';
    } else if (!feof(in)) {
        /* No newline yet. If the next character ends the line, the line
         * filled the buffer exactly; otherwise it is too long. */
        int c = getc(in);
        if (c != '\n' && c != EOF) {
            while ((c = getc(in)) != EOF && c != '\n') {
            }
            buf[0] = '\0';
            return DP_ERR_TOO_LONG;
        }
    }
    if (len > 0 && buf[len - 1] == '\r') {
        buf[len - 1] = '\0'; /* a file with Windows line endings */
    }
    return DP_OK;
}

dp_status dp_copy_string(char *dst, size_t size, const char *src)
{
    if (dst == NULL || src == NULL) {
        return DP_ERR_NULL;
    }
    if (size == 0) {
        return DP_ERR_TOO_LONG;
    }
    const size_t len = strlen(src);
    const size_t n = len < size ? len : size - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
    return n == len ? DP_OK : DP_ERR_TOO_LONG;
}

dp_status dp_mul_size(size_t a, size_t b, size_t *out)
{
    if (out == NULL) {
        return DP_ERR_NULL;
    }
    if (b != 0 && a > SIZE_MAX / b) {
        return DP_ERR_OVERFLOW;
    }
    *out = a * b;
    return DP_OK;
}

dp_status dp_alloc_array(size_t count, size_t size, void **out)
{
    if (out == NULL) {
        return DP_ERR_NULL;
    }
    size_t bytes;
    const dp_status st = dp_mul_size(count, size, &bytes);
    if (st != DP_OK) {
        return st;
    }
    void *p = calloc(count == 0 ? 1 : count, size == 0 ? 1 : size);
    if (p == NULL) {
        return DP_ERR_NO_MEMORY;
    }
    *out = p;
    return DP_OK;
}

dp_status dp_copy_file(const char *from_path, const char *to_path)
{
    if (from_path == NULL || to_path == NULL) {
        return DP_ERR_NULL;
    }
    dp_status status = DP_ERR_IO;
    FILE *in = NULL;
    FILE *out = NULL;
    int created = 0; /* set once to_path has been opened for writing */
    char buffer[4096];

    in = fopen(from_path, "rb");
    if (in == NULL) {
        goto cleanup;
    }
    out = fopen(to_path, "wb");
    if (out == NULL) {
        goto cleanup;
    }
    created = 1;
    size_t n;
    while ((n = fread(buffer, 1, sizeof buffer, in)) > 0) {
        if (fwrite(buffer, 1, n, out) != n) {
            goto cleanup;
        }
    }
    if (ferror(in)) {
        goto cleanup;
    }
    /* fclose flushes the last buffer, so its result matters too. */
    const int close_failed = fclose(out);
    out = NULL;
    if (close_failed != 0) {
        goto cleanup;
    }
    status = DP_OK;

cleanup:
    if (in != NULL) {
        fclose(in);
    }
    if (out != NULL) {
        fclose(out);
    }
    if (status != DP_OK && created) {
        const int saved = errno;
        remove(to_path); /* do not leave a half-written copy */
        errno = saved;
    }
    return status;
}
