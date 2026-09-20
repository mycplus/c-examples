/* ping.c - minimal ICMP echo client for Windows (Winsock 2).
   Build from a Developer Command Prompt:
       cl /W4 /WX /EHsc ping.c ws2_32.lib
   Run from an elevated prompt - raw sockets need Administrator.          */

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "Ws2_32.lib")

#define ICMP_ECHO        8
#define ICMP_ECHOREPLY   0

#define DEFAULT_PAYLOAD 56
#define MAX_PAYLOAD     1024
#define RECV_BUFFER     2048
#define PING_COUNT      4

/* Winsock supplies no ICMP or IP header definitions, so declare them.
   Both must be packed - the compiler would otherwise pad them. */
#pragma pack(push, 1)

typedef struct ip_header {
    uint8_t  ver_ihl;              /* version << 4 | header length in words */
    uint8_t  tos;
    uint16_t total_len;
    uint16_t ident;
    uint16_t flags_fragment;
    uint8_t  ttl;
    uint8_t  proto;
    uint16_t checksum;
    uint32_t src;                  /* fixed width, not unsigned long - the
    uint32_t dst;                     size of long differs between models */
} ip_header;

typedef struct icmp_header {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
} icmp_header;

#pragma pack(pop)

/* 16-bit one's complement sum, as RFC 1071 describes it. */
static uint16_t checksum16(const void *data, size_t len)
{
    const unsigned char *p = (const unsigned char *)data;
    uint32_t sum = 0;

    while (len > 1) {
        uint16_t word;
        memcpy(&word, p, sizeof word);
        sum += word;
        p   += 2;
        len -= 2;
    }
    if (len == 1)
        sum += *p;

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (uint16_t)~sum;
}

/* GetTickCount64 only resolves to about 15 ms, which reports every
   loopback reply as 0 ms. QueryPerformanceCounter is sub-microsecond. */
static double now_ms(void)
{
    static LARGE_INTEGER freq;
    LARGE_INTEGER now;

    if (freq.QuadPart == 0)
        QueryPerformanceFrequency(&freq);

    QueryPerformanceCounter(&now);
    return (double)now.QuadPart * 1000.0 / (double)freq.QuadPart;
}

static void report_socket_failure(int err)
{
    fprintf(stderr, "Could not open a raw ICMP socket (WSA error %d).\n", err);
    if (err == WSAEACCES)
        fprintf(stderr,
                "WSAEACCES usually means the process is not elevated.\n"
                "Right-click the command prompt and choose "
                "\"Run as administrator\", then try again.\n");
    else
        fprintf(stderr,
                "Raw sockets require Administrator privileges on Windows.\n");
}

int main(int argc, char **argv)
{
    WSADATA wsa;
    int rc = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (rc != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", rc);
        return 1;
    }

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s <host> [payload-bytes]\n", argv[0]);
        WSACleanup();
        return 2;
    }

    long payload = DEFAULT_PAYLOAD;
    if (argc == 3) {
        char *end;
        payload = strtol(argv[2], &end, 10);
        if (*end != '\0' || payload < 0 || payload > MAX_PAYLOAD) {
            fprintf(stderr, "payload must be between 0 and %d bytes\n", MAX_PAYLOAD);
            WSACleanup();
            return 2;
        }
    }

    /* getaddrinfo, not gethostbyname - the old resolver is deprecated
       on Windows as well as everywhere else. */
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_ICMP;

    struct addrinfo *res = NULL;
    rc = getaddrinfo(argv[1], NULL, &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "%s: %s\n", argv[1], gai_strerrorA(rc));
        WSACleanup();
        return 1;
    }
    struct sockaddr_in dest = *(struct sockaddr_in *)res->ai_addr;
    freeaddrinfo(res);

    char dotted[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &dest.sin_addr, dotted, sizeof dotted);

    /* WSA_FLAG_OVERLAPPED is required for SO_RCVTIMEO to take effect on a
       socket created with WSASocket. Without it the receive blocks forever. */
    SOCKET sock = WSASocketW(AF_INET, SOCK_RAW, IPPROTO_ICMP,
                             NULL, 0, WSA_FLAG_OVERLAPPED);
    if (sock == INVALID_SOCKET) {
        report_socket_failure(WSAGetLastError());
        WSACleanup();
        return 1;
    }

    DWORD timeout_ms = 1000;            /* a DWORD here, not a struct timeval */
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
               (const char *)&timeout_ms, sizeof timeout_ms);

    printf("PING %s (%s) %ld bytes of data [raw socket]\n",
           argv[1], dotted, payload);

    const size_t packet_len = sizeof(icmp_header) + (size_t)payload;
    unsigned char packet[sizeof(icmp_header) + MAX_PAYLOAD];
    unsigned char reply[RECV_BUFFER];

    const unsigned short ident = (unsigned short)GetCurrentProcessId();
    int received = 0;

    for (int seq = 0; seq < PING_COUNT; ++seq) {
        memset(packet, 'E', packet_len);

        icmp_header hdr;
        memset(&hdr, 0, sizeof hdr);
        hdr.type     = ICMP_ECHO;
        hdr.code     = 0;
        hdr.id       = htons(ident);
        hdr.sequence = htons((unsigned short)seq);
        memcpy(packet, &hdr, sizeof hdr);

        hdr.checksum = checksum16(packet, packet_len);
        memcpy(packet, &hdr, sizeof hdr);

        const double sent_at = now_ms();

        if (sendto(sock, (const char *)packet, (int)packet_len, 0,
                   (struct sockaddr *)&dest, sizeof dest) == SOCKET_ERROR) {
            fprintf(stderr, "sendto failed: %d\n", WSAGetLastError());
            break;
        }

        /* A raw socket can also hand back our own outgoing request, so read
           until the reply we asked for arrives or the timeout expires. */
        int got_reply = 0;
        while (!got_reply) {
            struct sockaddr_in from;
            int fromlen = sizeof from;
            int n = recvfrom(sock, (char *)reply, (int)sizeof reply, 0,
                             (struct sockaddr *)&from, &fromlen);
            if (n == SOCKET_ERROR) {
                int err = WSAGetLastError();
                if (err == WSAETIMEDOUT)
                    printf("seq=%d timeout\n", seq);
                else
                    fprintf(stderr, "recvfrom failed: %d\n", err);
                break;
            }

            if ((size_t)n < sizeof(ip_header))
                continue;

            const ip_header *ip = (const ip_header *)reply;
            const size_t offset = (size_t)(ip->ver_ihl & 0x0f) * 4;

            if ((size_t)n < offset + sizeof(icmp_header))
                continue;

            icmp_header in;
            memcpy(&in, reply + offset, sizeof in);

            if (in.type != ICMP_ECHOREPLY)
                continue;
            if (ntohs(in.id) != ident)
                continue;

            char fromdot[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &from.sin_addr, fromdot, sizeof fromdot);
            printf("%d bytes from %s: icmp_seq=%u time=%.2f ms\n",
                   n - (int)offset, fromdot, ntohs(in.sequence),
                   now_ms() - sent_at);
            ++received;
            got_reply = 1;
        }

        if (seq + 1 < PING_COUNT)
            Sleep(1000);
    }

    printf("--- %s statistics ---\n%d sent, %d received, %d%% loss\n",
           dotted, PING_COUNT, received,
           (PING_COUNT - received) * 100 / PING_COUNT);

    closesocket(sock);
    WSACleanup();
    return received > 0 ? 0 : 1;
}