/* base64.h - RFC 4648 base64 encoding and decoding.
 *
 * The caller owns every buffer. Nothing here allocates, keeps global
 * state, or needs cleaning up, so these functions are safe to call from
 * any thread.
 */
#ifndef MYCPLUS_BASE64_H
#define MYCPLUS_BASE64_H

#include <stddef.h>

#define B64_ERROR ((size_t)-1)

/* Bytes needed to hold the encoding of n input bytes, including the
   terminating NUL. Encoding 0 bytes needs 1 byte: the NUL itself. */
size_t b64_encoded_size(size_t n);

/* Upper bound on the bytes produced by decoding n characters. */
size_t b64_decoded_size(size_t n);

/* Encode len bytes from in into out, which must have room for at least
   b64_encoded_size(len) bytes. Writes a terminating NUL. Returns the
   number of characters written, not counting the NUL, or B64_ERROR if
   out is too small. */
size_t b64_encode(const unsigned char *in, size_t len,
                  char *out, size_t out_size);

/* Decode len characters from in into out, which must have room for at
   least b64_decoded_size(len) bytes. Returns the number of bytes
   written, or B64_ERROR if out is too small or the input is not valid
   base64 - invalid characters, misplaced padding and lengths that are
   not a multiple of four are all rejected. */
size_t b64_decode(const char *in, size_t len,
                  unsigned char *out, size_t out_size);

#endif /* MYCPLUS_BASE64_H */
