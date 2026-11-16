/* Sending to a port where nobody is listening: unconnected vs connected UDP. */
#include "udp.h"

#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

int main(void)
{
    if (!net_init())
        return 1;

    /* Find a free loopback port, then close it so nothing listens there. */
    struct sockaddr_in closed;
    memset(&closed, 0, sizeof closed);
    closed.sin_family = AF_INET;
    closed.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t len = sizeof closed;
    socket_t probe = socket(AF_INET, SOCK_DGRAM, 0);
    if (probe == BAD_SOCKET || bind(probe, (struct sockaddr *)&closed, len) != 0 ||
        getsockname(probe, (struct sockaddr *)&closed, &len) != 0)
        return 1;
    close_socket(probe);

    /* Unconnected: sendto() succeeds and nothing ever reports the failure. */
    socket_t a = socket(AF_INET, SOCK_DGRAM, 0);
    int n = (int)sendto(a, "hello", 5, 0, (struct sockaddr *)&closed, (socklen_t)sizeof closed);
    printf("unconnected sendto to a closed port returned %d\n", n);
    close_socket(a);

    /* Connected: the kernel remembers the peer and reports its ICMP error. */
    socket_t b = socket(AF_INET, SOCK_DGRAM, 0);
    if (connect(b, (struct sockaddr *)&closed, (socklen_t)sizeof closed) != 0)
        return 1;
    set_recv_timeout(b, 1000);
    n = (int)send(b, "hello", 5, 0);
    printf("connected send to the same port returned %d\n", n);
    char buf[16];
    n = (int)recv(b, buf, (int)sizeof buf, 0);
    int e = net_last_error();
    if (n < 0 && net_error_is_refused(e))
        printf("the next recv failed: port unreachable (nobody is listening)\n");
    else if (n < 0 && net_error_is_timeout(e))
        printf("the next recv timed out: no error was reported\n");
    else
        printf("the next recv returned %d (error %d)\n", n, e);
    close_socket(b);
    net_cleanup();
    return 0;
}
