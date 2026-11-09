/* Shows SLIP escaping and what the decoder does with damaged and noisy input. */
#include "frame.h"

#include <stdio.h>
#include <string.h>

static const char *result_name[] = {"none", "OK", "bad CRC", "too long", "bad escape", "empty"};

static void print_bytes(const char *label, const uint8_t *b, size_t n)
{
    printf("%-22s", label);
    for (size_t i = 0; i < n; i++)
        printf(" %02X", b[i]);
    printf("  (%zu bytes)\n", n);
}

static size_t encode_text(const char *text, uint8_t *out, size_t cap)
{
    return frame_encode((const uint8_t *)text, strlen(text), out, cap);
}

/* Feeds a byte stream through a fresh decoder and prints every result. */
static void receive(const char *label, const uint8_t *wire, size_t n)
{
    frame_decoder dec;
    frame_decoder_init(&dec);
    printf("%-30s ->", label);
    for (size_t i = 0; i < n; i++) {
        const uint8_t *msg = NULL;
        size_t len = 0;
        frame_result r = frame_decoder_feed(&dec, wire[i], &msg, &len);
        if (r == FRAME_OK)
            printf(" OK \"%.*s\"", (int)len, (const char *)msg);
        else if (r != FRAME_NONE)
            printf(" %s", result_name[r]);
    }
    printf("\n");
}

int main(void)
{
    const char *check = "123456789";
    printf("CRC-16/CCITT-FALSE of \"%s\" = 0x%04X\n\n", check,
           crc16_ccitt((const uint8_t *)check, strlen(check)));

    const uint8_t payload[] = {'H', 'i', FRAME_END, '!', FRAME_ESC};
    uint8_t wire[FRAME_ENCODED_MAX(FRAME_MAX_PAYLOAD) * 2];
    size_t n = frame_encode(payload, sizeof payload, wire, sizeof wire);
    print_bytes("payload:", payload, sizeof payload);
    print_bytes("on the wire:", wire, n);
    printf("\n");

    n = encode_text("PING", wire, sizeof wire);
    receive("clean frame", wire, n);

    wire[2] = (uint8_t)(wire[2] ^ 0x04);           /* flip one bit of the payload */
    receive("one bit flipped", wire, n);

    n = encode_text("PING", wire, sizeof wire);
    memmove(wire + 3, wire + 4, n - 4);
    receive("one byte lost", wire, n - 1);

    const uint8_t noise[] = {0x55, 0xAA, 0x13};
    memcpy(wire, noise, sizeof noise);
    n = sizeof noise + encode_text("PING", wire + sizeof noise, sizeof wire - sizeof noise);
    receive("noise before the frame", wire, n);

    n = encode_text("ONE", wire, sizeof wire);
    n += encode_text("TWO", wire + n, sizeof wire - n);
    receive("two frames back to back", wire, n);

    n = encode_text("ONE", wire, sizeof wire) - 1;   /* drop the closing END */
    n += encode_text("TWO", wire + n, sizeof wire - n);
    receive("closing END lost", wire, n);
    /* One damaged byte became END. Without the empty-frame rule, FF FF would pass
       as a zero-length message, because 0xFFFF is the CRC of no bytes at all. */
    const uint8_t moved_end[] = {FRAME_END, 0xFF, 0xFF, FRAME_END, FRAME_END};
    receive("a data byte turned into END", moved_end, sizeof moved_end);
    return 0;
}
