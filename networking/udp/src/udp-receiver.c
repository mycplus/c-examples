/* Receives datagrams on PORT and reports any gap in their sequence numbers.
   Stops after COUNT datagrams, or when none arrives for TIMEOUT_MS.
   Usage: udp-receiver PORT [COUNT] [TIMEOUT_MS] */
#include "udp.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s PORT [COUNT] [TIMEOUT_MS]\n", argv[0]);
        return 2;
    }
    long want = argc > 2 ? strtol(argv[2], NULL, 10) : 0;      /* 0: no limit */
    long timeout = argc > 3 ? strtol(argv[3], NULL, 10) : 5000;
    if (!net_init())
        return 1;

    char err[256];
    socket_t s = udp_bind(argv[1], err, sizeof err);
    if (s == BAD_SOCKET || !set_recv_timeout(s, (int)timeout)) {
        fprintf(stderr, "%s\n", s == BAD_SOCKET ? err : "cannot set the timeout");
        net_cleanup();
        return 1;
    }
    printf("listening on UDP port %s\n", argv[1]);
    fflush(stdout);

    long got = 0, expected = 1, lost = 0;
    while (want == 0 || got < want) {
        char buf[1500];                         /* larger than any datagram we expect */
        struct sockaddr_storage from;
        socklen_t from_len = sizeof from;
        int n = (int)recvfrom(s, buf, (int)(sizeof buf - 1), 0, (struct sockaddr *)&from, &from_len);
        if (n < 0) {
            int e = net_last_error();
            if (net_error_is_refused(e))
                continue;                       /* Windows: an old "port unreachable" */
            if (net_error_is_timeout(e))
                printf("no datagram for %ld ms, stopping\n", timeout);
            else
                fprintf(stderr, "recvfrom failed (error %d)\n", e);
            break;
        }
        buf[n] = '\0';                          /* the payload is text in this example */
        got++;

        char who[80];
        format_address((const struct sockaddr *)&from, who, sizeof who);
        long seq = strtol(buf, NULL, 10);
        if (seq > expected) {
            lost += seq - expected;
            printf("  (missing %ld datagram(s) before this one)\n", seq - expected);
        }
        if (seq >= expected)
            expected = seq + 1;
        printf("from %s, %d bytes: %s\n", who, n, buf);
        fflush(stdout);
    }
    printf("received %ld datagram(s), %ld missing\n", got, lost);
    close_socket(s);
    net_cleanup();
    return 0;
}
