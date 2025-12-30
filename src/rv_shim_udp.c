#include "rv_shim_udp.h"

#include <stdlib.h>
#include <string.h>

struct rv_shim_udp {
    rv_udp_socket_t* sock;
    rv_sockaddr_t relay;
    int started;
};

static int udp_send_cb(void* user, const uint8_t* data, uint32_t len) {
    rv_shim_udp_t* u = (rv_shim_udp_t*)user;
    if (!u || !u->sock || !data || len == 0) return -1;
    if (len > 0x7fffffffU) return -2;
    return rv_udp_sendto(u->sock, &u->relay, data, (int)len);
}

static int udp_recv_cb(void* user, uint8_t* out, uint32_t cap) {
    rv_shim_udp_t* u = (rv_shim_udp_t*)user;
    if (!u || !u->sock || !out || cap == 0) return -1;
    if (cap > 0x7fffffffU) return -2;

    int r = rv_udp_recvfrom(u->sock, NULL, out, (int)cap);
    if (r < 0) return r;

    // your rv_udp_recvfrom returns 0 when would-block (no data)
    return r;
}

rv_shim_udp_t* rv_shim_udp_create(const rv_shim_udp_config_t* cfg) {
    if (!cfg || !cfg->relay_host || cfg->relay_port == 0) return NULL;

    rv_shim_udp_t* u = (rv_shim_udp_t*)malloc(sizeof(*u));
    if (!u) return NULL;
    memset(u, 0, sizeof(*u));

    if (rv_udp_startup() != 0) {
        free(u);
        return NULL;
    }
    u->started = 1;

    uint16_t bind_port = cfg->local_port ? cfg->local_port : cfg->relay_port;
    u->sock = rv_udp_create_dualstack(bind_port, cfg->nonblocking ? 1 : 0);
    if (!u->sock) {
        rv_udp_cleanup();
        free(u);
        return NULL;
    }

    if (rv_parse_ip_port(cfg->relay_host, cfg->relay_port, &u->relay) != 0) {
        rv_udp_destroy(u->sock);
        rv_udp_cleanup();
        free(u);
        return NULL;
    }

    return u;
}

void rv_shim_udp_destroy(rv_shim_udp_t* u) {
    if (!u) return;
    if (u->sock) rv_udp_destroy(u->sock);
    if (u->started) rv_udp_cleanup();
    free(u);
}

rv_shim_transport_t rv_shim_udp_as_transport(rv_shim_udp_t* u) {
    rv_shim_transport_t t;
    t.user = u;
    t.send = udp_send_cb;
    t.recv = udp_recv_cb;
    return t;
}
