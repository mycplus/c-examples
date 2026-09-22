/* tcp-client.c - TCP echo client.
 *
 *   tcp-client <host> <port> <message> [count]
 *
 * Sends message count times and reads each echo back. Because TCP is a
 * byte stream, a reply is not guaranteed to arrive in a single recv();
 * the client reads until it has received exactly as many bytes as it
 * sent, then checks that they match.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "net.h"

/* Connect to host:port, trying each address getaddrinfo returns until
   one succeeds. Returns the descriptor, or -1. */
static int connect_to(const char *host, const char *port)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;             /* IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *res;
    int rc = getaddrinfo(host, port, &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "%s: %s\n", host, gai_strerror(rc));
        return -1;
    }

    int fd = -1;
    for (struct addrinfo *ai = res; ai != NULL; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0)
            continue;
        if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0)
            break;                              /* connected */
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);

    if (fd < 0)
        fprintf(stderr, "could not connect to %s:%s\n", host, port);
    return fd;
}

int main(int argc, char **argv)
{
    if (argc < 4 || argc > 5) {
        fprintf(stderr, "usage: %s <host> <port> <message> [count]\n", argv[0]);
        return 2;
    }

    long count = 1;
    if (argc == 5) {
        char *end;
        errno = 0;
        count = strtol(argv[4], &end, 10);
        if (errno != 0 || *end != '\0' || count < 1 || count > 1000000) {
            fprintf(stderr, "count must be between 1 and 1000000\n");
            return 2;
        }
    }

    signal(SIGPIPE, SIG_IGN);

    /* argv strings are NUL-terminated C strings, so strlen is the right
       length here. For arbitrary binary data you would carry the length
       separately - strlen stops at the first zero byte. */
    const char  *msg = argv[3];
    const size_t len = strlen(msg);

    char *reply = malloc(len ? len : 1);
    if (reply == NULL) {
        fputs("out of memory\n", stderr);
        return 1;
    }

    const int fd = connect_to(argv[1], argv[2]);
    if (fd < 0) {
        free(reply);
        return 1;
    }

    int status = 0;
    for (long i = 0; i < count; ++i) {
        if (send_all(fd, msg, len) < 0) {
            perror("send");
            status = 1;
            break;
        }

        const ssize_t got = recv_exact(fd, reply, len);
        if (got < 0) {
            perror("recv");
            status = 1;
            break;
        }
        if ((size_t)got != len) {
            fprintf(stderr, "server closed after %zd of %zu bytes\n", got, len);
            status = 1;
            break;
        }
        if (memcmp(reply, msg, len) != 0) {
            fprintf(stderr, "echo did not match on message %ld\n", i + 1);
            status = 1;
            break;
        }
    }

    if (status == 0)
        printf("sent and verified %ld message(s) of %zu bytes\n", count, len);

    close(fd);
    free(reply);
    return status;
}
