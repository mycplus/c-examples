#define _POSIX_C_SOURCE 200809L
/* Two-way text chat over a serial port. Every line you type is sent as one
   framed message; every valid frame that arrives is printed.
   Usage: chat DEVICE [BAUD]   e.g.  chat /dev/ttyUSB0 115200   or   chat COM3 */
#include "frame.h"
#include "serial.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <pthread.h>
#include <time.h>
#endif

static serial_port *port;

/* Runs on its own thread so that typing and receiving never block each other.
   Returns only when the port fails or the other end disconnects. */
static void receive_loop(void)
{
    frame_decoder dec;
    frame_decoder_init(&dec);
    unsigned long damaged = 0;
    uint8_t buf[256];

    long n;
    while ((n = serial_read(port, buf, sizeof buf, 200)) >= 0) {
        for (long i = 0; i < n; i++) {
            const uint8_t *msg = NULL;
            size_t len = 0;
            frame_result r = frame_decoder_feed(&dec, buf[i], &msg, &len);
            if (r == FRAME_OK) {
                printf("peer> %.*s\n", (int)len, (const char *)msg);
                fflush(stdout);
            } else if (r != FRAME_NONE) {
                fprintf(stderr, "(dropped a damaged frame; %lu so far)\n", ++damaged);
            }
        }
    }
    fprintf(stderr, "serial port closed or failed; nothing more will be received\n");
}

#ifdef _WIN32
static DWORD WINAPI reader(LPVOID arg)
{
    (void)arg;
    receive_loop();
    return 0;
}
#else
static void *reader(void *arg)
{
    (void)arg;
    receive_loop();
    return NULL;
}
#endif

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s DEVICE [BAUD]\n", argv[0]);
        return 2;
    }
    long baud = argc > 2 ? strtol(argv[2], NULL, 10) : 115200;

    char err[256];
    port = serial_open(argv[1], baud, err, sizeof err);
    if (port == NULL) {
        fprintf(stderr, "%s\n", err);
        return 1;
    }

#ifdef _WIN32
    if (CreateThread(NULL, 0, reader, NULL, 0, NULL) == NULL) {
#else
    pthread_t tid;
    if (pthread_create(&tid, NULL, reader, NULL) != 0) {
#endif
        fprintf(stderr, "cannot start the receive thread\n");
        return 1;
    }

    printf("connected to %s at %ld baud; type a line and press Enter\n", argv[1], baud);
    fflush(stdout);

    char line[FRAME_MAX_PAYLOAD + 1];
    uint8_t wire[FRAME_ENCODED_MAX(FRAME_MAX_PAYLOAD)];
    while (fgets(line, sizeof line, stdin) != NULL) {
        size_t len = strcspn(line, "\r\n");
        if (len == 0)
            continue;
        size_t n = frame_encode((const uint8_t *)line, len, wire, sizeof wire);
        if (n == 0 || !serial_write_all(port, wire, n)) {
            fprintf(stderr, "send failed\n");
            return 1;
        }
    }

    /* End of input: give replies already on the wire a moment to print. The
       receive thread is still using the port, so the OS closes it at exit. */
#ifdef _WIN32
    Sleep(300);
#else
    nanosleep(&(struct timespec){.tv_sec = 0, .tv_nsec = 300000000}, NULL);
#endif
    return 0;
}
