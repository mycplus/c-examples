#ifndef UDP_H
#define UDP_H

/* A thin layer over BSD sockets (Linux, macOS) and Winsock 2 (Windows), so the
   example programs read the same on every platform. */

#ifndef _WIN32
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L    /* getaddrinfo, nanosleep, recvmsg */
#endif
#endif

#include <stdbool.h>
#include <stddef.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
#define BAD_SOCKET INVALID_SOCKET
#else
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
typedef int socket_t;
#define BAD_SOCKET (-1)
#endif

bool net_init(void);                 /* WSAStartup on Windows; nothing elsewhere */
void net_cleanup(void);
void close_socket(socket_t s);
int net_last_error(void);            /* errno, or WSAGetLastError() */
bool net_error_is_timeout(int err);
bool net_error_is_refused(int err);  /* the peer answered "port unreachable" */

/* Waits at most ms milliseconds in recvfrom() before failing with a timeout. */
bool set_recv_timeout(socket_t s, int ms);

/* A socket bound to port on every local address: IPv6 dual-stack when
   available, IPv4 otherwise. Returns BAD_SOCKET and fills err on failure. */
socket_t udp_bind(const char *port, char *err, size_t err_len);

/* Resolves host and port and opens a matching socket. The destination is
   stored in *dest for sendto(). */
socket_t udp_open(const char *host, const char *port, struct sockaddr_storage *dest,
                  socklen_t *dest_len, char *err, size_t err_len);

/* "127.0.0.1:5150" or "[::1]:5150" */
void format_address(const struct sockaddr *sa, char *out, size_t out_len);

#endif
