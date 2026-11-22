/* defensive.h - small, checked building blocks for C programs.
 *
 * Every function returns a dp_status. Results go through an output pointer,
 * which is written only on success, so a caller can never use a value that
 * was not produced.
 */
#ifndef DEFENSIVE_H
#define DEFENSIVE_H

#include <stddef.h>
#include <stdio.h>

/* Ask the compiler to warn when a caller ignores the returned status. */
#if defined(__GNUC__) || defined(__clang__)
#define DP_NODISCARD __attribute__((warn_unused_result))
#else
#define DP_NODISCARD
#endif

typedef enum {
    DP_OK = 0,
    DP_ERR_NULL,          /* a required pointer argument was NULL */
    DP_ERR_EMPTY,         /* the input was empty */
    DP_ERR_NOT_A_NUMBER,  /* no digits where a number was expected */
    DP_ERR_TRAILING,      /* a number followed by other characters */
    DP_ERR_RANGE,         /* a number outside the allowed range */
    DP_ERR_TOO_LONG,      /* the input does not fit the buffer */
    DP_ERR_EOF,           /* no more input */
    DP_ERR_OVERFLOW,      /* a size calculation would overflow */
    DP_ERR_NO_MEMORY,     /* malloc or calloc failed */
    DP_ERR_IO             /* a file operation failed; see errno */
} dp_status;

/* A short English description of a status code. Never returns NULL. */
const char *dp_strerror(dp_status status);

/* Parses all of text as a base-10 integer in [min, max].
 * Leading spaces are allowed; anything after the number is not. */
DP_NODISCARD dp_status dp_parse_long(const char *text, long min, long max, long *out);

/* Reads one line into buf without the newline. A line that does not fit is
 * consumed up to its newline and reported as DP_ERR_TOO_LONG. */
DP_NODISCARD dp_status dp_read_line(FILE *in, char *buf, size_t size);

/* Copies src into dst (size bytes, always terminated). Returns
 * DP_ERR_TOO_LONG if src was cut short. */
DP_NODISCARD dp_status dp_copy_string(char *dst, size_t size, const char *src);

/* *out = a * b, or DP_ERR_OVERFLOW if the product does not fit in size_t. */
DP_NODISCARD dp_status dp_mul_size(size_t a, size_t b, size_t *out);

/* Allocates count elements of size bytes each, zeroed, with an overflow check. */
DP_NODISCARD dp_status dp_alloc_array(size_t count, size_t size, void **out);

/* Copies the file at from_path to to_path. On failure, errno describes
 * the failing call and no partial output file is left behind. */
DP_NODISCARD dp_status dp_copy_file(const char *from_path, const char *to_path);

#endif
