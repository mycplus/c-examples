#ifndef FRAME_H
#define FRAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* SLIP framing (RFC 1055) with a CRC-16 appended to every message. */
#define FRAME_END      0xC0
#define FRAME_ESC      0xDB
#define FRAME_ESC_END  0xDC
#define FRAME_ESC_ESC  0xDD

#define FRAME_MAX_PAYLOAD 256
/* Worst case: every byte of payload and CRC escaped, plus two END bytes. */
#define FRAME_ENCODED_MAX(n) (2 + 2 * ((n) + 2))

/* CRC-16/CCITT-FALSE: polynomial 0x1021, initial value 0xFFFF. */
uint16_t crc16_ccitt(const uint8_t *data, size_t len);

/* Returns the number of bytes written to out, or 0 if len is 0 or too large, or out_cap is too small. */
size_t frame_encode(const uint8_t *payload, size_t len, uint8_t *out, size_t out_cap);

typedef enum {
    FRAME_NONE,        /* no complete frame yet */
    FRAME_OK,          /* a valid frame: see payload and len */
    FRAME_BAD_CRC,     /* a frame arrived but its CRC did not match: discarded */
    FRAME_TOO_LONG,    /* more bytes than FRAME_MAX_PAYLOAD + 2 before END: discarded */
    FRAME_BAD_ESCAPE,  /* ESC followed by something other than ESC_END or ESC_ESC */
    FRAME_EMPTY        /* a CRC with no payload: messages are 1 to FRAME_MAX_PAYLOAD bytes */
} frame_result;

typedef struct {
    uint8_t buf[FRAME_MAX_PAYLOAD + 2];
    size_t len;
    bool escaped;
    bool broken;       /* the frame in progress is already known to be bad */
    frame_result broken_reason;
} frame_decoder;

void frame_decoder_init(frame_decoder *d);

/* Feeds one received byte. On FRAME_OK, *payload and *len describe the message
   (valid until the next call). Bytes before the first END are treated as noise. */
frame_result frame_decoder_feed(frame_decoder *d, uint8_t byte, const uint8_t **payload, size_t *len);

#endif
