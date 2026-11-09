/* Unit tests for frame.c: CRC, encoding limits, round trips and damaged input. */
#include "frame.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;
static void check(bool ok, const char *expr, int line)
{
    if (!ok) {
        fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, line, expr);
        failures++;
    }
}
#define CHECK(cond) check((cond), #cond, __LINE__)

static unsigned long long rng = 88172645463325252ULL;
static uint8_t rand_byte(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return (uint8_t)rng;
}

/* An independent, table-driven CRC to cross-check the bitwise one. */
static uint16_t table[256];
static void build_table(void)
{
    for (unsigned i = 0; i < 256; i++) {
        uint16_t c = (uint16_t)(i << 8);
        for (int b = 0; b < 8; b++)
            c = (c & 0x8000) ? (uint16_t)((c << 1) ^ 0x1021) : (uint16_t)(c << 1);
        table[i] = c;
    }
}
static uint16_t crc_table(const uint8_t *d, size_t n)
{
    uint16_t crc = 0xFFFF;
    while (n--)
        crc = (uint16_t)((crc << 8) ^ table[((crc >> 8) ^ *d++) & 0xFF]);
    return crc;
}

/* Feeds n bytes; returns how many FRAME_OK results matched want/want_len. */
static int decode_count(const uint8_t *wire, size_t n, const uint8_t *want, size_t want_len,
                        int *other_results)
{
    frame_decoder d;
    frame_decoder_init(&d);
    int ok = 0;
    *other_results = 0;
    for (size_t i = 0; i < n; i++) {
        const uint8_t *msg = NULL;
        size_t len = 0;
        frame_result r = frame_decoder_feed(&d, wire[i], &msg, &len);
        if (r == FRAME_OK && len == want_len && memcmp(msg, want, len) == 0)
            ok++;
        else if (r != FRAME_NONE)
            (*other_results)++;
    }
    return ok;
}

static void test_crc(void)
{
    build_table();
    CHECK(crc16_ccitt((const uint8_t *)"123456789", 9) == 0x29B1);  /* published check value */
    CHECK(crc16_ccitt(NULL, 0) == 0xFFFF);
    uint8_t data[300];
    for (int round = 0; round < 2000; round++) {
        size_t n = (size_t)(rand_byte()) + (size_t)(rand_byte() % 45);
        for (size_t i = 0; i < n; i++)
            data[i] = rand_byte();
        CHECK(crc16_ccitt(data, n) == crc_table(data, n));
    }
}

static void test_encode_limits(void)
{
    uint8_t p[FRAME_MAX_PAYLOAD + 1] = {0};
    uint8_t out[FRAME_ENCODED_MAX(FRAME_MAX_PAYLOAD + 1)];
    CHECK(frame_encode(p, 0, out, sizeof out) == 0);
    CHECK(frame_encode(p, FRAME_MAX_PAYLOAD + 1, out, sizeof out) == 0);
    CHECK(frame_encode(p, 10, out, FRAME_ENCODED_MAX(10) - 1) == 0);
    CHECK(frame_encode(p, 10, out, FRAME_ENCODED_MAX(10)) > 0);
}

static void test_round_trips(void)
{
    uint8_t p[FRAME_MAX_PAYLOAD];
    uint8_t wire[FRAME_ENCODED_MAX(FRAME_MAX_PAYLOAD)];
    for (size_t len = 1; len <= FRAME_MAX_PAYLOAD; len++) {
        for (int pattern = 0; pattern < 4; pattern++) {
            for (size_t i = 0; i < len; i++)
                p[i] = (uint8_t)(pattern == 0 ? rand_byte() : pattern == 1 ? FRAME_END
                                 : pattern == 2 ? FRAME_ESC : i);
            size_t n = frame_encode(p, len, wire, sizeof wire);
            CHECK(n > 0 && n <= FRAME_ENCODED_MAX(len));
            CHECK(wire[0] == FRAME_END && wire[n - 1] == FRAME_END);
            for (size_t i = 1; i + 1 < n; i++)
                CHECK(wire[i] != FRAME_END);           /* END never appears inside a frame */
            int other;
            CHECK(decode_count(wire, n, p, len, &other) == 1 && other == 0);
        }
    }
}

static void test_damaged_input(void)
{
    uint8_t p[32], wire[FRAME_ENCODED_MAX(32)], copy[sizeof wire];
    for (size_t i = 0; i < sizeof p; i++)
        p[i] = rand_byte();
    size_t n = frame_encode(p, sizeof p, wire, sizeof wire);
    int other;

    /* every possible single-bit error between the END bytes is rejected */
    for (size_t bit = 8; bit < (n - 1) * 8; bit++) {
        memcpy(copy, wire, n);
        copy[bit / 8] = (uint8_t)(copy[bit / 8] ^ (1u << (bit % 8)));
        CHECK(decode_count(copy, n, p, sizeof p, &other) == 0);
    }

    /* a frame that is too long is dropped, and the next one still arrives */
    uint8_t stream[600];
    size_t s = 0;
    stream[s++] = FRAME_END;
    for (int i = 0; i < 300; i++)
        stream[s++] = 'x';
    memcpy(stream + s, wire, n);
    s += n;
    frame_decoder d;
    frame_decoder_init(&d);
    int too_long = 0, ok = 0;
    for (size_t i = 0; i < s; i++) {
        const uint8_t *msg = NULL;
        size_t len = 0;
        frame_result r = frame_decoder_feed(&d, stream[i], &msg, &len);
        too_long += r == FRAME_TOO_LONG;
        ok += r == FRAME_OK && len == sizeof p && memcmp(msg, p, len) == 0;
    }
    CHECK(too_long == 1 && ok == 1);

    /* the size limit is exact: 256 payload bytes pass, 257 are too long */
    for (size_t plen = FRAME_MAX_PAYLOAD; plen <= FRAME_MAX_PAYLOAD + 1; plen++) {
        uint8_t big[FRAME_MAX_PAYLOAD + 1], raw[FRAME_MAX_PAYLOAD + 5];
        memset(big, 'a', sizeof big);
        uint16_t crc = crc16_ccitt(big, plen);
        size_t r = 0;
        raw[r++] = FRAME_END;
        memcpy(raw + r, big, plen);
        r += plen;
        raw[r++] = (uint8_t)(crc >> 8);       /* this CRC has no END or ESC bytes */
        raw[r++] = (uint8_t)(crc & 0xFF);
        raw[r++] = FRAME_END;
        CHECK(raw[r - 3] != FRAME_END && raw[r - 3] != FRAME_ESC);
        CHECK(raw[r - 2] != FRAME_END && raw[r - 2] != FRAME_ESC);
        frame_decoder_init(&d);
        frame_result last = FRAME_NONE;
        for (size_t i = 0; i < r; i++) {
            const uint8_t *msg = NULL;
            size_t len = 0;
            frame_result fr = frame_decoder_feed(&d, raw[i], &msg, &len);
            if (fr != FRAME_NONE)
                last = fr;
        }
        CHECK(last == (plen == FRAME_MAX_PAYLOAD ? FRAME_OK : FRAME_TOO_LONG));
    }

    /* small edge cases, each fed to a fresh decoder */
    const uint8_t idle[] = {FRAME_END, FRAME_END, FRAME_END};
    const uint8_t one_byte[] = {FRAME_END, 0x41, FRAME_END};
    const uint8_t crc_only[] = {FRAME_END, 0xFF, 0xFF, FRAME_END};
    const uint8_t bad_escape[] = {FRAME_END, 0x41, FRAME_ESC, 0x41, 0x42, FRAME_END};
    const uint8_t esc_then_end[] = {FRAME_END, 0x41, FRAME_ESC, FRAME_END};
    const struct {
        const uint8_t *bytes;
        size_t n;
        frame_result want;
    } cases[] = {
        {idle, sizeof idle, FRAME_NONE},
        {one_byte, sizeof one_byte, FRAME_BAD_CRC},
        {crc_only, sizeof crc_only, FRAME_EMPTY},
        {bad_escape, sizeof bad_escape, FRAME_BAD_ESCAPE},
        {esc_then_end, sizeof esc_then_end, FRAME_BAD_ESCAPE},
    };
    for (size_t c = 0; c < sizeof cases / sizeof cases[0]; c++) {
        frame_decoder_init(&d);
        frame_result last = FRAME_NONE;
        for (size_t i = 0; i < cases[c].n; i++) {
            const uint8_t *msg = NULL;
            size_t len = 0;
            frame_result r = frame_decoder_feed(&d, cases[c].bytes[i], &msg, &len);
            if (r != FRAME_NONE)
                last = r;
        }
        CHECK(last == cases[c].want);
    }
}

int main(void)
{
    test_crc();
    test_encode_limits();
    test_round_trips();
    test_damaged_input();
    if (failures == 0)
        printf("test_frame: all checks passed\n");
    return failures != 0;
}
