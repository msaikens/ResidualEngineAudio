#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
static void rv_sleep_ms(int ms) { Sleep((DWORD)ms); }
#else
#include <unistd.h>
static void rv_sleep_ms(int ms) { usleep(ms * 1000); }
#endif

#include "rv_udp.h"
#include "rv_netproto.h"
#include "rv_relay.h"

static int udp_send_wrap(void* ctx, const rv_sockaddr_t* to, const uint8_t* data, int len) {
    rv_udp_socket_t* s = (rv_udp_socket_t*)ctx;
    return rv_udp_sendto(s, to, data, len);
}

static uint16_t parse_u16(const char* s, uint16_t def) {
    if (!s || !*s) return def;
    long v = strtol(s, NULL, 10);
    if (v < 0 || v > 65535) return def;
    return (uint16_t)v;
}

int main(int argc, char** argv) {
    uint16_t port = 40000;
    if (argc >= 2) port = parse_u16(argv[1], 40000);

    if (rv_udp_startup() != 0) {
        printf("relay: WSAStartup failed\n");
        return 1;
    }

    rv_udp_socket_t* sock = rv_udp_create_dualstack(port, 1 /*nonblocking*/);
    if (!sock) {
        printf("relay: failed to bind UDP port %u\n", (unsigned)port);
        rv_udp_cleanup();
        return 2;
    }

    rv_relay_state_t st;
    rv_relay_init(&st);

    printf("residual_relay listening on UDP port %u (dual-stack)\n", (unsigned)port);

    uint8_t buf[1400];

    for (;;) {
        rv_sockaddr_t from;
        int r = rv_udp_recvfrom(sock, &from, buf, (int)sizeof(buf));
        if (r <= 0) {
            rv_sleep_ms(1);
            continue;
        }

        rv_pkt_hdr_t hdr;
        if (rv_parse_packet_header(buf, r, &hdr) != 0) continue;

        if (hdr.type == RV_PKT_JOIN) {
            uint64_t session_id = 0;
            uint16_t player_id = 0;
            if (rv_parse_join_payload(buf, r, &session_id, &player_id) == 0) {
                rv_relay_join(&st, session_id, player_id, &from);
                printf("JOIN session=%llu player=%u\n",
                       (unsigned long long)session_id, (unsigned)player_id);
            }
        } else if (hdr.type == RV_PKT_VOICE) {
            // Prototype routing: single session for now
            const uint64_t session_id = 1234;

            // ✅ FIX: pass &from (address), NOT speaker_id or player_id
            rv_relay_forward_voice(&st, session_id, &from, buf, r, sock, udp_send_wrap);

            printf("VOICE len=%d from speaker=%u seq=%u\n",
                   r, (unsigned)hdr.speaker_id, (unsigned)hdr.seq);
        }
    }
}
