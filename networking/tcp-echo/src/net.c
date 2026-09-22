#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <sys/socket.h>

#include "net.h"

/* MSG_NOSIGNAL stops send() raising SIGPIPE when the peer has closed the
   connection, so the error arrives as EPIPE instead of killing the
   process. macOS lacks it and uses the SO_NOSIGPIPE socket option. */
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

int send_all(int fd, const void *buf, size_t len)
{
    const char *p = buf;

    while (len > 0) {
        ssize_t n = send(fd, p, len, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR)
                continue;               /* interrupted before sending */
            return -1;
        }
        p   += n;                       /* send() may write fewer bytes */
        len -= (size_t)n;               /* than asked; send the rest   */
    }
    return 0;
}

ssize_t recv_exact(int fd, void *buf, size_t len)
{
    char  *p   = buf;
    size_t got = 0;

    while (got < len) {
        ssize_t n = recv(fd, p + got, len - got, 0);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0)
            break;                      /* peer closed: return what we have */
        got += (size_t)n;
    }
    return (ssize_t)got;
}
