/* net.h - helpers for reading and writing a TCP byte stream. */
#ifndef MYCPLUS_NET_H
#define MYCPLUS_NET_H

#include <stddef.h>
#include <sys/types.h>

/* Send all len bytes, retrying on partial writes and EINTR.
   Returns 0 on success, -1 on error (errno is set). */
int send_all(int fd, const void *buf, size_t len);

/* Receive exactly len bytes, retrying on short reads and EINTR.
   Returns the number of bytes received: len on success, fewer if the
   peer closed the connection first, or -1 on error (errno is set). */
ssize_t recv_exact(int fd, void *buf, size_t len);

#endif /* MYCPLUS_NET_H */
