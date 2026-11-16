/* What a datagram socket preserves: message boundaries, empty messages, and
   what happens when the receive buffer is too small. Loopback only. */
#include "udp.h"

#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

/* A socket bound to 127.0.0.1 on a port the system picks; *addr gets that port. */
static socket_t loopback_socket(struct sockaddr_in *addr)
{
    socket_t s = socket(AF_INET, SOCK_DGRAM, 0);
    memset(addr, 0, sizeof *addr);
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t len = sizeof *addr;
    if (s == BAD_SOCKET || bind(s, (struct sockaddr *)addr, len) != 0 ||
        getsockname(s, (struct sockaddr *)addr, &len) != 0)
        return BAD_SOCKET;
    set_recv_timeout(s, 1000);
    return s;
}

/* Returns the bytes kept, and sets *truncated if the datagram did not fit. */
static int receive(socket_t s, char *buf, int cap, bool *truncated)
{
#ifdef _WIN32
    int n = recv(s, buf, cap, 0);
    *truncated = n < 0 && WSAGetLastError() == WSAEMSGSIZE;
    return *truncated ? cap : n;
#else
    struct iovec iov = {.iov_base = buf, .iov_len = (size_t)cap};
    struct msghdr msg;
    memset(&msg, 0, sizeof msg);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    int n = (int)recvmsg(s, &msg, 0);
    *truncated = n >= 0 && (msg.msg_flags & MSG_TRUNC);
    return n;
#endif
}

int main(void)
{
    if (!net_init())
        return 1;
    struct sockaddr_in to;
    socket_t rx = loopback_socket(&to);
    socket_t tx = socket(AF_INET, SOCK_DGRAM, 0);
    if (rx == BAD_SOCKET || tx == BAD_SOCKET) {
        fprintf(stderr, "cannot open loopback sockets\n");
        return 1;
    }

    char big[300];
    memset(big, 'x', sizeof big);
    const struct { const char *data; int len; } sends[] = {
        {"hello", 5}, {"!", 1}, {big, 300}, {"", 0},
    };
    for (size_t i = 0; i < sizeof sends / sizeof sends[0]; i++)
        sendto(tx, sends[i].data, sends[i].len, 0, (struct sockaddr *)&to, (socklen_t)sizeof to);

    printf("sent datagrams of 5, 1, 300 and 0 bytes\n");
    for (size_t i = 0; i < sizeof sends / sizeof sends[0]; i++) {
        char buf[2048];
        bool cut;
        int n = receive(rx, buf, (int)sizeof buf, &cut);
        printf("receive %zu returned %d bytes\n", i + 1, n);
    }

    /* The same 300-byte datagram into a 4-byte buffer. */
    sendto(tx, big, 300, 0, (struct sockaddr *)&to, (socklen_t)sizeof to);
    sendto(tx, "next", 4, 0, (struct sockaddr *)&to, (socklen_t)sizeof to);
    char small[4];
    bool cut = false;
    int n = receive(rx, small, (int)sizeof small, &cut);
    printf("300 bytes into a 4-byte buffer: kept %d, truncated: %s\n", n, cut ? "yes" : "no");
    char buf[2048];
    n = receive(rx, buf, (int)sizeof buf, &cut);
    printf("the next receive returned the next datagram: %d bytes, \"%.*s\"\n", n, n, buf);

    close_socket(tx);
    close_socket(rx);
    net_cleanup();
    return 0;
}
