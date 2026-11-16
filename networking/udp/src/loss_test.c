/* Sends COUNT datagrams to a socket that is not reading yet, then counts how
   many survived. Usage: loss_test [COUNT] [RCVBUF_BYTES]   (0 = system default) */
#include "udp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

int main(int argc, char **argv)
{
    long count = argc > 1 ? strtol(argv[1], NULL, 10) : 10000;
    int rcvbuf = argc > 2 ? (int)strtol(argv[2], NULL, 10) : 0;
    if (count < 1 || !net_init())
        return 2;

    struct sockaddr_in to;
    memset(&to, 0, sizeof to);
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t len = sizeof to;
    socket_t rx = socket(AF_INET, SOCK_DGRAM, 0);
    socket_t tx = socket(AF_INET, SOCK_DGRAM, 0);
    if (rx == BAD_SOCKET || tx == BAD_SOCKET)
        return 1;
    if (rcvbuf > 0)
        setsockopt(rx, SOL_SOCKET, SO_RCVBUF, (const char *)&rcvbuf, (socklen_t)sizeof rcvbuf);
    if (bind(rx, (struct sockaddr *)&to, len) != 0 || getsockname(rx, (struct sockaddr *)&to, &len) != 0)
        return 1;
    int actual = 0;
    socklen_t optlen = sizeof actual;
    getsockopt(rx, SOL_SOCKET, SO_RCVBUF, (char *)&actual, &optlen);

    /* Blast every datagram before the receiver reads anything. */
    long send_errors = 0;
    for (long seq = 1; seq <= count; seq++) {
        char msg[64];
        int n = snprintf(msg, sizeof msg, "%ld reading", seq);
        if (sendto(tx, msg, n, 0, (struct sockaddr *)&to, (socklen_t)sizeof to) != n)
            send_errors++;
    }

    set_recv_timeout(rx, 200);
    long got = 0, last = 0, out_of_order = 0;
    char buf[128];
    int n;
    while ((n = (int)recv(rx, buf, (int)(sizeof buf - 1), 0)) > 0) {
        buf[n] = '\0';
        long seq = strtol(buf, NULL, 10);
        if (seq < 1 || seq > count || strstr(buf, " reading") == NULL)
            return 1;                                 /* corrupted: never expected */
        if (seq <= last)
            out_of_order++;
        last = seq;
        got++;
    }

    printf("receive buffer: requested %s, actual %d bytes\n", rcvbuf > 0 ? argv[2] : "default", actual);
    printf("sent %ld, send errors %ld, received %ld, lost %ld (%.1f%%), out of order %ld\n",
           count, send_errors, got, count - got, 100.0 * (double)(count - got) / (double)count,
           out_of_order);
    close_socket(tx);
    close_socket(rx);
    net_cleanup();
    return got > 0 && got <= count && out_of_order == 0 ? 0 : 1;
}
