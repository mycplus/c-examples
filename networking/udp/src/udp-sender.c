/* Sends COUNT numbered datagrams to HOST:PORT, one every DELAY milliseconds.
   Usage: udp-sender HOST PORT MESSAGE [COUNT] [DELAY_MS]
   Example: udp-sender 127.0.0.1 5150 "boiler temp=71.3C" 5 */
#include "udp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define sleep_ms(ms) Sleep((DWORD)(ms))
#else
#include <time.h>
static void sleep_ms(long ms)
{
    struct timespec t = {.tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L};
    nanosleep(&t, NULL);
}
#endif

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "usage: %s HOST PORT MESSAGE [COUNT] [DELAY_MS]\n", argv[0]);
        return 2;
    }
    long count = argc > 4 ? strtol(argv[4], NULL, 10) : 1;
    long delay = argc > 5 ? strtol(argv[5], NULL, 10) : 100;
    if (count < 1 || delay < 0) {
        fprintf(stderr, "COUNT must be at least 1 and DELAY_MS not negative\n");
        return 2;
    }
    if (!net_init())
        return 1;

    char err[256];
    struct sockaddr_storage dest;
    socklen_t dest_len = 0;
    socket_t s = udp_open(argv[1], argv[2], &dest, &dest_len, err, sizeof err);
    if (s == BAD_SOCKET) {
        fprintf(stderr, "%s\n", err);
        net_cleanup();
        return 1;
    }

    char where[80];
    format_address((const struct sockaddr *)&dest, where, sizeof where);
    for (long seq = 1; seq <= count; seq++) {
        char datagram[1024];
        int n = snprintf(datagram, sizeof datagram, "%ld %s", seq, argv[3]);
        if (n < 0 || (size_t)n >= sizeof datagram) {
            fprintf(stderr, "message too long\n");
            break;
        }
        /* One sendto() is one datagram: it arrives whole or not at all. */
        if (sendto(s, datagram, n, 0, (const struct sockaddr *)&dest, dest_len) != n) {
            fprintf(stderr, "sendto failed (error %d)\n", net_last_error());
            break;
        }
        printf("sent %d bytes to %s: %s\n", n, where, datagram);
        fflush(stdout);
        if (seq < count)
            sleep_ms(delay);
    }
    close_socket(s);
    net_cleanup();
    return 0;
}
