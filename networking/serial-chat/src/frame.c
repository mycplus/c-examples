#include "frame.h"

uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    while (len--) {
        crc = (uint16_t)(crc ^ (*data++ << 8));
        for (int bit = 0; bit < 8; bit++)
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
    }
    return crc;
}

static size_t put_escaped(uint8_t byte, uint8_t *out)
{
    if (byte == FRAME_END) {
        out[0] = FRAME_ESC;
        out[1] = FRAME_ESC_END;
        return 2;
    }
    if (byte == FRAME_ESC) {
        out[0] = FRAME_ESC;
        out[1] = FRAME_ESC_ESC;
        return 2;
    }
    out[0] = byte;
    return 1;
}

size_t frame_encode(const uint8_t *payload, size_t len, uint8_t *out, size_t out_cap)
{
    if (len == 0 || len > FRAME_MAX_PAYLOAD || out_cap < FRAME_ENCODED_MAX(len))
        return 0;

    uint16_t crc = crc16_ccitt(payload, len);
    size_t n = 0;
    out[n++] = FRAME_END;              /* a leading END flushes any line noise at the receiver */
    for (size_t i = 0; i < len; i++)
        n += put_escaped(payload[i], out + n);
    n += put_escaped((uint8_t)(crc >> 8), out + n);
    n += put_escaped((uint8_t)(crc & 0xFF), out + n);
    out[n++] = FRAME_END;
    return n;
}

void frame_decoder_init(frame_decoder *d)
{
    d->len = 0;
    d->escaped = false;
    d->broken = false;
    d->broken_reason = FRAME_NONE;
}

static void mark_broken(frame_decoder *d, frame_result why)
{
    if (!d->broken) {
        d->broken = true;
        d->broken_reason = why;
    }
}

frame_result frame_decoder_feed(frame_decoder *d, uint8_t byte, const uint8_t **payload, size_t *len)
{
    if (byte == FRAME_END) {
        frame_result result = FRAME_NONE;
        if (d->broken) {
            result = d->broken_reason;
        } else if (d->escaped) {
            result = FRAME_BAD_ESCAPE;
        } else if (d->len == 2) {
            result = FRAME_EMPTY;      /* never sent: a damaged byte became END */
        } else if (d->len > 2) {
            size_t n = d->len - 2;
            uint16_t sent = (uint16_t)((d->buf[n] << 8) | d->buf[n + 1]);
            if (crc16_ccitt(d->buf, n) == sent) {
                *payload = d->buf;
                *len = n;
                result = FRAME_OK;
            } else {
                result = FRAME_BAD_CRC;
            }
        } else if (d->len == 1) {
            result = FRAME_BAD_CRC;    /* one stray byte between two END bytes */
        }
        d->len = 0;                    /* an empty frame (END END) is just idle line */
        d->escaped = false;
        d->broken = false;
        return result;
    }

    if (d->broken)
        return FRAME_NONE;             /* skip to the next END */

    if (d->escaped) {
        d->escaped = false;
        if (byte == FRAME_ESC_END)
            byte = FRAME_END;
        else if (byte == FRAME_ESC_ESC)
            byte = FRAME_ESC;
        else {
            mark_broken(d, FRAME_BAD_ESCAPE);
            return FRAME_NONE;
        }
    } else if (byte == FRAME_ESC) {
        d->escaped = true;
        return FRAME_NONE;
    }

    if (d->len == sizeof d->buf) {
        mark_broken(d, FRAME_TOO_LONG);
        return FRAME_NONE;
    }
    d->buf[d->len++] = byte;
    return FRAME_NONE;
}
