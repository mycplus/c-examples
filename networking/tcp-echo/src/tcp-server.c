/* tcp-server.c - TCP echo server, one thread per client.
 *
 *   tcp-server <port>
 *
 * Echoes every byte it receives back to the sender, unchanged. The data
 * is treated as bytes, not text, so it may contain anything, including
 * zero bytes.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "net.h"

#define BUFFER_SIZE 4096

static void *handle_client(void *arg)
{
    const int fd = *(int *)arg;
    free(arg);

    char buf[BUFFER_SIZE];

    for (;;) {
        ssize_t n = recv(fd, buf, sizeof buf, 0);
        if (n == 0)
            break;                              /* client closed */
        if (n < 0) {
            if (errno == EINTR)
                continue;
            perror("recv");
            break;
        }
        if (send_all(fd, buf, (size_t)n) < 0) {
            perror("send");
            break;
        }
    }

    close(fd);
    return NULL;
}

/* Bind a socket of the given family to port on the wildcard address.
   Returns the descriptor, or -1. */
static int bind_family(const char *port, int family)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = family;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;            /* wildcard address */

    struct addrinfo *res;
    if (getaddrinfo(NULL, port, &hints, &res) != 0)
        return -1;

    int fd = -1;
    for (struct addrinfo *ai = res; ai != NULL; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0)
            continue;

        /* Allow an immediate restart while old connections sit in
           TIME_WAIT; without this, bind() fails with EADDRINUSE. */
        int one = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);

        /* On an IPv6 socket, ask for dual-stack: accept IPv4 clients too,
           as IPv4-mapped addresses. Where the system does not allow this,
           the socket stays IPv6-only. */
        if (ai->ai_family == AF_INET6) {
            int zero = 0;
            setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &zero, sizeof zero);
        }

        if (bind(fd, ai->ai_addr, ai->ai_addrlen) == 0)
            break;                              /* bound */

        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    return fd;
}

/* Prefer an IPv6 dual-stack socket, which serves both IPv4 and IPv6;
   fall back to IPv4 alone on a system without IPv6. */
static int open_listener(const char *port)
{
    int fd = bind_family(port, AF_INET6);
    if (fd < 0)
        fd = bind_family(port, AF_INET);
    if (fd < 0) {
        fprintf(stderr, "could not bind to port %s\n", port);
        return -1;
    }
    if (listen(fd, SOMAXCONN) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }
    return fd;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return 2;
    }

    /* Writing to a socket the peer has closed raises SIGPIPE, which
       terminates the process by default. Ignore it and handle EPIPE. */
    signal(SIGPIPE, SIG_IGN);

    const int listener = open_listener(argv[1]);
    if (listener < 0)
        return 1;

    printf("listening on port %s\n", argv[1]);
    fflush(stdout);

    for (;;) {
        int fd = accept(listener, NULL, NULL);
        if (fd < 0) {
            if (errno == EINTR)
                continue;
            perror("accept");
            continue;                           /* keep serving others */
        }

        /* Pass the descriptor through allocated memory, not by casting
           an int to a pointer: each thread gets its own copy. */
        int *arg = malloc(sizeof *arg);
        if (arg == NULL) {
            close(fd);
            continue;
        }
        *arg = fd;

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, arg) != 0) {
            perror("pthread_create");
            free(arg);
            close(fd);
            continue;
        }
        pthread_detach(tid);                    /* reclaimed on exit */
    }
}
