/* ping.c - minimal ICMP echo client for Linux and macOS.
   Build: cc -std=c11 -Wall -Wextra -pedantic ping.c -o ping          */

#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_PAYLOAD 56
#define MAX_PAYLOAD     1024
#define RECV_BUFFER     2048
#define PING_COUNT      4

/* 16-bit one's complement sum, as RFC 1071 describes it. */
static unsigned short checksum(const void *data, size_t len)
{
    const unsigned char *p = data;
    unsigned long sum = 0;

    while (len > 1) {
        unsigned short word;
        memcpy(&word, p, sizeof word);   /* no aliasing games */
        sum += word;
        p   += 2;
        len -= 2;
    }
    if (len == 1)
        sum += *p;

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (unsigned short)~sum;
}

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

/* Try the unprivileged socket first, fall back to the raw one.
   *is_raw tells the caller whether replies will carry an IP header. */
static int open_icmp_socket(int *is_raw)
{
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if (fd >= 0) {
        *is_raw = 0;
        return fd;
    }
    const int dgram_errno = errno;

    fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (fd >= 0) {
        *is_raw = 1;
        return fd;
    }

    fprintf(stderr,
            "Could not open an ICMP socket.\n"
            "  SOCK_DGRAM: %s\n"
            "  SOCK_RAW  : %s\n"
            "Either run as root, grant CAP_NET_RAW with\n"
            "  sudo setcap cap_net_raw+ep ./ping\n"
            "or allow unprivileged ICMP for your group with\n"
            "  sudo sysctl -w net.ipv4.ping_group_range=\"0 2147483647\"\n",
            strerror(dgram_errno), strerror(errno));
    return -1;
}

int main(int argc, char **argv)
{
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s <host> [payload-bytes]\n", argv[0]);
        return 2;
    }

    long payload = DEFAULT_PAYLOAD;
    if (argc == 3) {
        char *end;
        payload = strtol(argv[2], &end, 10);
        if (*end != '\0' || payload < 0 || payload > MAX_PAYLOAD) {
            fprintf(stderr, "payload must be between 0 and %d bytes\n", MAX_PAYLOAD);
            return 2;
        }
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_RAW;

    struct addrinfo *res = NULL;
    int rc = getaddrinfo(argv[1], NULL, &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "%s: %s\n", argv[1], gai_strerror(rc));
        return 1;
    }
    struct sockaddr_in dest = *(struct sockaddr_in *)res->ai_addr;
    freeaddrinfo(res);

    char dotted[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &dest.sin_addr, dotted, sizeof dotted);

    int is_raw = 0;
    int fd = open_icmp_socket(&is_raw);
    if (fd < 0)
        return 1;

    struct timeval tv = { 1, 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);

    printf("PING %s (%s) %ld bytes of data [%s socket]\n",
           argv[1], dotted, payload, is_raw ? "raw" : "unprivileged");

    const size_t packet_len = sizeof(struct icmphdr) + (size_t)payload;
    unsigned char packet[sizeof(struct icmphdr) + MAX_PAYLOAD];
    unsigned char reply[RECV_BUFFER];

    const unsigned short ident = (unsigned short)getpid();
    int received = 0;

    for (int seq = 0; seq < PING_COUNT; ++seq) {
        memset(packet, 'E', packet_len);

        struct icmphdr hdr;
        memset(&hdr, 0, sizeof hdr);
        hdr.type             = ICMP_ECHO;
        hdr.code             = 0;
        hdr.un.echo.id       = htons(ident);
        hdr.un.echo.sequence = htons((unsigned short)seq);
        memcpy(packet, &hdr, sizeof hdr);

        hdr.checksum = checksum(packet, packet_len);
        memcpy(packet, &hdr, sizeof hdr);

        const double sent_at = now_ms();

        if (sendto(fd, packet, packet_len, 0,
                   (struct sockaddr *)&dest, sizeof dest) < 0) {
            fprintf(stderr, "sendto: %s\n", strerror(errno));
            break;
        }

        /* A raw socket on loopback also receives our own outgoing echo
           request, so keep reading until the reply we asked for turns up
           or the timeout expires. */
        int got_reply = 0;
        while (!got_reply) {
            struct sockaddr_in from;
            socklen_t fromlen = sizeof from;
            ssize_t n = recvfrom(fd, reply, sizeof reply, 0,
                                 (struct sockaddr *)&from, &fromlen);
            if (n < 0) {
                printf("seq=%d timeout\n", seq);
                break;
            }

            /* A raw socket hands back the IP header; SOCK_DGRAM does not. */
            size_t offset = 0;
            if (is_raw) {
                if ((size_t)n < sizeof(struct iphdr)) continue;
                const struct iphdr *ip = (const struct iphdr *)reply;
                offset = (size_t)ip->ihl * 4;
            }
            if ((size_t)n < offset + sizeof(struct icmphdr))
                continue;

            struct icmphdr in;
            memcpy(&in, reply + offset, sizeof in);

            if (in.type != ICMP_ECHOREPLY)
                continue;                       /* our own request, or something else */

            /* The kernel rewrites the id on an unprivileged socket, so only
               a raw socket can match on it. */
            if (is_raw && ntohs(in.un.echo.id) != ident)
                continue;                       /* somebody else's ping */

            char fromdot[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &from.sin_addr, fromdot, sizeof fromdot);
            printf("%zd bytes from %s: icmp_seq=%u time=%.2f ms\n",
                   n - (ssize_t)offset, fromdot,
                   ntohs(in.un.echo.sequence), now_ms() - sent_at);
            ++received;
            got_reply = 1;
        }

        if (seq + 1 < PING_COUNT)
            sleep(1);
    }

    printf("--- %s statistics ---\n%d sent, %d received, %d%% loss\n",
           dotted, PING_COUNT, received,
           (PING_COUNT - received) * 100 / PING_COUNT);

    close(fd);
    return received > 0 ? 0 : 1;
}