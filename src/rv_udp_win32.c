#define WIN32_LEAN_AND_MEAN
#include "rv_udp.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <string.h>

struct rv_udp_socket {
    SOCKET sock;
};

int rv_udp_startup(void) {
    WSADATA w;
    return (WSAStartup(MAKEWORD(2,2), &w) == 0) ? 0 : -1;
}

void rv_udp_cleanup(void) {
    WSACleanup();
}

static int set_nonblocking(SOCKET s, int nb) {
    u_long mode = nb ? 1 : 0;
    return (ioctlsocket(s, FIONBIO, &mode) == 0) ? 0 : -1;
}

rv_udp_socket_t* rv_udp_create_dualstack(uint16_t bind_port, int nonblocking) {
    SOCKET s = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return NULL;

    // Allow v4-mapped addresses (dual stack)
    DWORD v6only = 0;
    setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&v6only, sizeof(v6only));

    // reuse addr
    BOOL reuse = TRUE;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(bind_port);

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        closesocket(s);
        return NULL;
    }

    if (nonblocking) set_nonblocking(s, 1);

    rv_udp_socket_t* out = (rv_udp_socket_t*)malloc(sizeof(*out));
    if (!out) { closesocket(s); return NULL; }
    out->sock = s;
    return out;
}

void rv_udp_destroy(rv_udp_socket_t* s) {
    if (!s) return;
    closesocket(s->sock);
    free(s);
}

static int to_sockaddr(const rv_sockaddr_t* in, struct sockaddr_storage* ss, int* sslen) {
    memset(ss, 0, sizeof(*ss));
    if (in->ver == RV_IPV4) {
        struct sockaddr_in* a = (struct sockaddr_in*)ss;
        a->sin_family = AF_INET;
        memcpy(&a->sin_addr, in->ip.v4, 4);
        a->sin_port = htons(in->port);
        *sslen = sizeof(*a);
        return 0;
    } else if (in->ver == RV_IPV6) {
        struct sockaddr_in6* a = (struct sockaddr_in6*)ss;
        a->sin6_family = AF_INET6;
        memcpy(&a->sin6_addr, in->ip.v6, 16);
        a->sin6_port = htons(in->port);
        *sslen = sizeof(*a);
        return 0;
    }
    return -1;
}

static int from_sockaddr(const struct sockaddr_storage* ss, int sslen, rv_sockaddr_t* out) {
    (void)sslen;
    if (ss->ss_family == AF_INET) {
        const struct sockaddr_in* a = (const struct sockaddr_in*)ss;
        out->ver = RV_IPV4;
        out->port = ntohs(a->sin_port);
        memcpy(out->ip.v4, &a->sin_addr, 4);
        return 0;
    } else if (ss->ss_family == AF_INET6) {
        const struct sockaddr_in6* a = (const struct sockaddr_in6*)ss;
        out->ver = RV_IPV6;
        out->port = ntohs(a->sin6_port);
        memcpy(out->ip.v6, &a->sin6_addr, 16);
        return 0;
    }
    return -1;
}

int rv_udp_sendto(rv_udp_socket_t* s, const rv_sockaddr_t* to, const uint8_t* data, int len) {
    if (!s || !to || !data || len <= 0) return -1;
    struct sockaddr_storage ss;
    int sslen = 0;
    if (to_sockaddr(to, &ss, &sslen) != 0) return -2;
    int r = sendto(s->sock, (const char*)data, len, 0, (struct sockaddr*)&ss, sslen);
    if (r == SOCKET_ERROR) return -3;
    return r;
}

int rv_udp_recvfrom(rv_udp_socket_t* s, rv_sockaddr_t* from, uint8_t* out, int out_cap) {
    if (!s || !out || out_cap <= 0) return -1;

    struct sockaddr_storage ss;
    int sslen = sizeof(ss);
    int r = recvfrom(s->sock, (char*)out, out_cap, 0, (struct sockaddr*)&ss, &sslen);
    if (r == SOCKET_ERROR) {
        int e = WSAGetLastError();
        if (e == WSAEWOULDBLOCK) return 0; // no data
        return -2;
    }

    if (from) (void)from_sockaddr(&ss, sslen, from);
    return r;
}

int rv_parse_ip_port(const char* host, uint16_t port, rv_sockaddr_t* out) {
    if (!host || !out) return -1;

    // IPv6 literal?
    struct in6_addr a6;
    if (InetPtonA(AF_INET6, host, &a6) == 1) {
        out->ver = RV_IPV6;
        out->port = port;
        memcpy(out->ip.v6, &a6, 16);
        return 0;
    }

    // IPv4 literal?
    struct in_addr a4;
    if (InetPtonA(AF_INET, host, &a4) == 1) {
        out->ver = RV_IPV4;
        out->port = port;
        memcpy(out->ip.v4, &a4, 4);
        return 0;
    }

    return -2;
}
