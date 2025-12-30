#include "rv_transport_udp.h"

#include <stdlib.h>
#include <string.h>

struct rv_transport_udp {
    rv_udp_socket_t* sock;
    rv_sockaddr_t    relay;
    int              started; // whether rv_udp_startup succeeded
};

rv_transport_udp_t* rv_transport_udp_create(const rv_transport_udp_config_t* cfg) {
    if (!cfg || !cfg->relay_host || cfg->relay_port == 0) return NULL;

    rv_transport_udp_t* t = (rv_transport_udp_t*)malloc(sizeof(*t));
    if (!t) return NULL;
    memset(t, 0, sizeof(*t));

    if (rv_udp_startup() != 0) {
        free(t);
        return NULL;
    }
    t->started = 1;

    uint16_t bind_port = cfg->local_port ? cfg->local_port : cfg->relay_port;

    t->sock = rv_udp_create_dualstack(bind_port, cfg->nonblocking ? 1 : 0);
    if (!t->sock) {
        rv_udp_cleanup();
        free(t);
        return NULL;
    }

    if (rv_parse_ip_port(cfg->relay_host, cfg->relay_port, &t->relay) != 0) {
        rv_udp_destroy(t->sock);
        rv_udp_cleanup();
        free(t);
        return NULL;
    }

    return t;
}

void rv_transport_udp_destroy(rv_transport_udp_t* t) {
    if (!t) return;
    if (t->sock) rv_udp_destroy(t->sock);
    if (t->started) rv_udp_cleanup();
    free(t);
}

int rv_transport_udp_sendto_relay(rv_transport_udp_t* t, const uint8_t* data, uint32_t len) {
    if (!t || !t->sock || !data || len == 0) return -1;
    if (len > 0x7fffffffU) return -2;
    return rv_udp_sendto(t->sock, &t->relay, data, (int)len);
}

int rv_transport_udp_recv(rv_transport_udp_t* t, uint8_t* out, uint32_t cap, uint32_t* out_len) {
    if (!t || !t->sock || !out || cap == 0 || !out_len) return -1;
    if (cap > 0x7fffffffU) return -2;

    int r = rv_udp_recvfrom(t->sock, NULL, out, (int)cap);
    if (r < 0) return r;
    *out_len = (uint32_t)r;
    return r;
}

int rv_transport_udp_pump_voice_outgoing(rv_transport_udp_t* t, rv_voice_t* v) {
    if (!t || !v) return -1;

    uint8_t pkt[1400];
    uint32_t pkt_len = 0;

    int sent = 0;
    for (;;) {
        int r = rv_voice_poll_outgoing(v, pkt, (uint32_t)sizeof(pkt), &pkt_len);
        if (r == 0) break;        // no packets
        if (r < 0) return -2;     // voice error / buffer too small

        int s = rv_transport_udp_sendto_relay(t, pkt, pkt_len);
        if (s < 0) return -3;     // udp error
        sent++;
    }

    return sent;
}

int rv_transport_udp_pump_voice_incoming(rv_transport_udp_t* t, rv_voice_t* v, uint32_t now_ms) {
    if (!t || !v) return -1;

    uint8_t buf[1400];
    int ingested = 0;

    for (;;) {
        uint32_t len = 0;
        int r = rv_transport_udp_recv(t, buf, (uint32_t)sizeof(buf), &len);
        if (r == 0) break;        // no data (nonblocking)
        if (r < 0) return -2;     // recv error

        (void)rv_voice_ingest_packet(v, buf, len, now_ms);
        ingested++;
    }

    return ingested;
}
