#include "udp.h"

#include <stdio.h>
#include <string.h>

#ifndef _WIN32
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#endif

bool net_init(void)
{
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
#else
    return true;
#endif
}

void net_cleanup(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

void close_socket(socket_t s)
{
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}

int net_last_error(void)
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

bool net_error_is_timeout(int err)
{
#ifdef _WIN32
    return err == WSAETIMEDOUT;
#else
    return err == EAGAIN || err == EWOULDBLOCK;
#endif
}

bool net_error_is_refused(int err)
{
#ifdef _WIN32
    return err == WSAECONNRESET || err == WSAECONNREFUSED;
#else
    return err == ECONNREFUSED;
#endif
}

bool set_recv_timeout(socket_t s, int ms)
{
#ifdef _WIN32
    DWORD t = (DWORD)ms;
    return setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&t, (socklen_t)sizeof t) == 0;
#else
    struct timeval t = {.tv_sec = ms / 1000, .tv_usec = (ms % 1000) * 1000};
    return setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof t) == 0;
#endif
}

socket_t udp_bind(const char *port, char *err, size_t err_len)
{
    static const int families[] = {AF_INET6, AF_INET};
    for (size_t i = 0; i < sizeof families / sizeof families[0]; i++) {
        struct addrinfo hints, *res = NULL;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = families[i];
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE;           /* the "any" address */
        if (getaddrinfo(NULL, port, &hints, &res) != 0)
            continue;

        socket_t s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (s != BAD_SOCKET) {
            if (res->ai_family == AF_INET6) {
                int off = 0;                   /* accept IPv4 too (dual-stack) */
                setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&off, (socklen_t)sizeof off);
            }
            if (bind(s, res->ai_addr, (socklen_t)res->ai_addrlen) == 0) {
                freeaddrinfo(res);
                return s;
            }
            close_socket(s);
        }
        freeaddrinfo(res);
    }
    snprintf(err, err_len, "cannot bind UDP port %s (error %d)", port, net_last_error());
    return BAD_SOCKET;
}

socket_t udp_open(const char *host, const char *port, struct sockaddr_storage *dest,
                  socklen_t *dest_len, char *err, size_t err_len)
{
    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;               /* IPv4 or IPv6, whatever host resolves to */
    hints.ai_socktype = SOCK_DGRAM;
    int rc = getaddrinfo(host, port, &hints, &res);
    if (rc != 0) {
        snprintf(err, err_len, "cannot resolve %s: %s", host, gai_strerror(rc));
        return BAD_SOCKET;
    }
    socket_t s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (s == BAD_SOCKET) {
        snprintf(err, err_len, "socket() failed (error %d)", net_last_error());
    } else {
        memcpy(dest, res->ai_addr, res->ai_addrlen);
        *dest_len = (socklen_t)res->ai_addrlen;
    }
    freeaddrinfo(res);
    return s;
}

void format_address(const struct sockaddr *sa, char *out, size_t out_len)
{
    char ip[INET6_ADDRSTRLEN] = "?";
    unsigned port = 0;
    if (sa->sa_family == AF_INET) {
        const struct sockaddr_in *v4 = (const struct sockaddr_in *)(const void *)sa;
        inet_ntop(AF_INET, &v4->sin_addr, ip, sizeof ip);
        port = ntohs(v4->sin_port);
        snprintf(out, out_len, "%s:%u", ip, port);
    } else if (sa->sa_family == AF_INET6) {
        const struct sockaddr_in6 *v6 = (const struct sockaddr_in6 *)(const void *)sa;
        inet_ntop(AF_INET6, &v6->sin6_addr, ip, sizeof ip);
        port = ntohs(v6->sin6_port);
        snprintf(out, out_len, "[%s]:%u", ip, port);
    } else {
        snprintf(out, out_len, "(unknown address family)");
    }
}
