#pragma once
#include <stdint.h>

typedef enum rv_ipver {
    RV_IPV4 = 4,
    RV_IPV6 = 6
} rv_ipver_t;

typedef struct rv_sockaddr {
    rv_ipver_t ver;
    uint16_t port; // host order
    union {
        uint8_t v4[4];   // network order bytes
        uint8_t v6[16];  // network order bytes
    } ip;
} rv_sockaddr_t;

typedef struct rv_udp_socket rv_udp_socket_t;

int rv_udp_startup(void);
void rv_udp_cleanup(void);

rv_udp_socket_t* rv_udp_create_dualstack(uint16_t bind_port, int nonblocking);
void rv_udp_destroy(rv_udp_socket_t* s);

int rv_udp_sendto(rv_udp_socket_t* s, const rv_sockaddr_t* to, const uint8_t* data, int len);
int rv_udp_recvfrom(rv_udp_socket_t* s, rv_sockaddr_t* from, uint8_t* out, int out_cap);

int rv_parse_ip_port(const char* host, uint16_t port, rv_sockaddr_t* out);
