/* Runs the serial code against a pseudo-terminal pair, so no cable is needed.
   The pty's slave side is opened with serial_open() as "PC A"; the master side
   plays "PC B". Linux and macOS only. */
#define _DEFAULT_SOURCE
#include "frame.h"
#include "serial.h"

#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <util.h>
#else
#include <pty.h>
#endif

/* Reads from fd until one complete frame arrives or 1 second passes. */
static frame_result receive_frame_fd(int fd, uint8_t *out, size_t *out_len)
{
    frame_decoder dec;
    frame_decoder_init(&dec);
    uint8_t byte;
    struct pollfd pfd = {.fd = fd, .events = POLLIN};
    while (poll(&pfd, 1, 1000) > 0 && read(fd, &byte, 1) == 1) {
        const uint8_t *msg = NULL;
        size_t len = 0;
        frame_result r = frame_decoder_feed(&dec, byte, &msg, &len);
        if (r == FRAME_OK) {
            memcpy(out, msg, len);
            *out_len = len;
        }
        if (r != FRAME_NONE)
            return r;
    }
    return FRAME_NONE;
}

int main(void)
{
    int master, slave;
    char name[128];
    if (openpty(&master, &slave, name, NULL, NULL) != 0) {
        perror("openpty");
        return 1;
    }

    char err[256];
    serial_port *a = serial_open(name, 115200, err, sizeof err);
    close(slave);                      /* serial_open holds its own descriptor now */
    if (a == NULL) {
        fprintf(stderr, "%s\n", err);
        return 1;
    }

    /* A -> B: a text message. */
    uint8_t wire[FRAME_ENCODED_MAX(FRAME_MAX_PAYLOAD)];
    const char *hello = "hello from A";
    size_t n = frame_encode((const uint8_t *)hello, strlen(hello), wire, sizeof wire);
    serial_write_all(a, wire, n);
    uint8_t got[FRAME_MAX_PAYLOAD];
    size_t got_len = 0;
    if (receive_frame_fd(master, got, &got_len) == FRAME_OK)
        printf("B received: \"%.*s\"\n", (int)got_len, (const char *)got);

    /* B -> A: bytes a terminal in cooked mode would change or swallow. */
    const uint8_t tricky[] = {'O', 'K', 0x0D, 0x0A, 0x03, 0x11, 0x13, 0x7F, 0x00, FRAME_END, FRAME_ESC};
    n = frame_encode(tricky, sizeof tricky, wire, sizeof wire);
    if (write(master, wire, n) != (ssize_t)n)
        return 1;

    frame_decoder dec;
    frame_decoder_init(&dec);
    uint8_t buf[64];
    int done = 0;
    while (!done) {
        long r = serial_read(a, buf, sizeof buf, 1000);
        if (r <= 0)
            break;
        for (long i = 0; i < r && !done; i++) {
            const uint8_t *msg = NULL;
            size_t len = 0;
            if (frame_decoder_feed(&dec, buf[i], &msg, &len) == FRAME_OK) {
                printf("A received %zu bytes, identical to what B sent: %s\n", len,
                       len == sizeof tricky && memcmp(msg, tricky, len) == 0 ? "yes" : "NO");
                done = 1;
            }
        }
    }

    /* Nothing more is coming: serial_read must give up after its timeout. */
    printf("serial_read with nothing to receive returned %ld\n", serial_read(a, buf, sizeof buf, 100));

    serial_close(a);
    close(master);
    return 0;
}
