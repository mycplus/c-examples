/* Damages encoded frames on purpose and counts how often a wrong message gets through.
   Usage: crc_experiment [scale]   (scale divides the trial counts; default 1) */
#include "frame.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long long rng = 0x9E3779B97F4A7C15ULL;
static unsigned long long next_rand(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return rng;
}
static size_t below(size_t n) { return (size_t)(next_rand() % n); }

enum { FLIP_1, FLIP_2, BURST_16, REPLACE_4 };

static void corrupt(int kind, uint8_t *wire, size_t n)
{
    /* Damage only the bytes between the opening and closing END. */
    size_t bits = (n - 2) * 8;
    switch (kind) {
    case FLIP_1:
    case FLIP_2:
    {
        size_t first = below(bits), second = first;
        while (kind == FLIP_2 && second == first)
            second = below(bits);              /* two different bits */
        wire[1 + first / 8] = (uint8_t)(wire[1 + first / 8] ^ (1u << (first % 8)));
        if (kind == FLIP_2)
            wire[1 + second / 8] = (uint8_t)(wire[1 + second / 8] ^ (1u << (second % 8)));
        break;
    }
    case BURST_16: {
        size_t len = 2 + below(15);                   /* 2..16 bits, first and last flipped */
        if (len > bits)
            len = bits;
        size_t start = below(bits - len + 1);
        for (size_t i = 0; i < len; i++)
            if (i == 0 || i == len - 1 || (next_rand() & 1)) {
                size_t b = start + i;
                wire[1 + b / 8] = (uint8_t)(wire[1 + b / 8] ^ (0x80u >> (b % 8)));
            }
        break;
    }
    case REPLACE_4: {
        size_t len = n - 2 < 4 ? n - 2 : 4;
        size_t start = 1 + below(n - 2 - len + 1);
        for (size_t i = 0; i < len; i++)
            wire[start + i] = (uint8_t)next_rand();
        break;
    }
    }
}

int main(int argc, char **argv)
{
    long scale = argc > 1 ? strtol(argv[1], NULL, 10) : 1;
    if (scale < 1)
        scale = 1;
    static const struct {
        const char *name;
        int kind;
        long trials;
    } rows[] = {
        {"1 bit flipped", FLIP_1, 200000},
        {"2 bits flipped", FLIP_2, 200000},
        {"burst of 2-16 bits", BURST_16, 200000},
        {"4 bytes replaced at random", REPLACE_4, 2000000},
    };

    printf("payloads: 1-64 random bytes; damage applied to the encoded bytes on the wire\n\n");
    printf("%-28s %9s %9s %9s %11s\n", "damage", "trials", "rejected", "intact", "WRONG MSG");
    for (size_t r = 0; r < sizeof rows / sizeof rows[0]; r++) {
        long trials = rows[r].trials / scale, rejected = 0, intact = 0, wrong = 0;
        for (long t = 0; t < trials; t++) {
            uint8_t payload[64];
            size_t len = 1 + below(64);
            for (size_t i = 0; i < len; i++)
                payload[i] = (uint8_t)next_rand();
            uint8_t wire[FRAME_ENCODED_MAX(64)];
            size_t n = frame_encode(payload, len, wire, sizeof wire);
            corrupt(rows[r].kind, wire, n);

            frame_decoder dec;
            frame_decoder_init(&dec);
            int got_wrong = 0, got_intact = 0;
            for (size_t i = 0; i < n; i++) {
                const uint8_t *msg = NULL;
                size_t mlen = 0;
                if (frame_decoder_feed(&dec, wire[i], &msg, &mlen) == FRAME_OK) {
                    if (mlen == len && memcmp(msg, payload, len) == 0)
                        got_intact = 1;
                    else
                        got_wrong = 1;
                }
            }
            if (got_wrong)
                wrong++;
            else if (got_intact)
                intact++;
            else
                rejected++;
        }
        printf("%-28s %9ld %9ld %9ld %11ld\n", rows[r].name, trials, rejected, intact, wrong);
    }
    printf("\nexpected WRONG MSG for random damage: about trials / 65536 = %.1f\n", 2000000.0 / scale / 65536.0);
    return 0;
}
