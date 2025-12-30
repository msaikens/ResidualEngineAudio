#include "rv_relay.h"
#include <string.h>

static int addr_equal(const rv_sockaddr_t* a, const rv_sockaddr_t* b) {
    if (a->ver != b->ver) return 0;
    if (a->port != b->port) return 0;
    if (a->ver == RV_IPV4) return memcmp(a->ip.v4, b->ip.v4, 4) == 0;
    return memcmp(a->ip.v6, b->ip.v6, 16) == 0;
}

static rv_relay_session_t* find_or_create_session(rv_relay_state_t* st, uint64_t session_id) {
    for (int i = 0; i < RV_RELAY_MAX_SESSIONS; i++) {
        if (st->sessions[i].in_use && st->sessions[i].session_id == session_id) {
            return &st->sessions[i];
        }
    }
    for (int i = 0; i < RV_RELAY_MAX_SESSIONS; i++) {
        if (!st->sessions[i].in_use) {
            st->sessions[i].in_use = 1;
            st->sessions[i].session_id = session_id;
            memset(st->sessions[i].clients, 0, sizeof(st->sessions[i].clients));
            return &st->sessions[i];
        }
    }
    return NULL;
}

void rv_relay_init(rv_relay_state_t* st) {
    memset(st, 0, sizeof(*st));
}

void rv_relay_join(rv_relay_state_t* st, uint64_t session_id, uint16_t player_id, const rv_sockaddr_t* from) {
    rv_relay_session_t* s = find_or_create_session(st, session_id);
    if (!s) return;

    // Update existing by player_id
    for (int i = 0; i < RV_RELAY_MAX_CLIENTS_PER_SESSION; i++) {
        if (s->clients[i].in_use && s->clients[i].player_id == player_id) {
            s->clients[i].addr = *from;
            return;
        }
    }

    // Or update existing by addr (client restarted)
    for (int i = 0; i < RV_RELAY_MAX_CLIENTS_PER_SESSION; i++) {
        if (s->clients[i].in_use && addr_equal(&s->clients[i].addr, from)) {
            s->clients[i].player_id = player_id;
            return;
        }
    }

    // Insert new
    for (int i = 0; i < RV_RELAY_MAX_CLIENTS_PER_SESSION; i++) {
        if (!s->clients[i].in_use) {
            s->clients[i].in_use = 1;
            s->clients[i].player_id = player_id;
            s->clients[i].addr = *from;
            return;
        }
    }
}

void rv_relay_forward_voice(rv_relay_state_t* st,
                            uint64_t session_id,
                            const rv_sockaddr_t* from,
                            const uint8_t* pkt,
                            int pkt_len,
                            void* send_ctx,
                            int (*send_fn)(void* ctx, const rv_sockaddr_t* to, const uint8_t* data, int len)) {
    rv_relay_session_t* s = NULL;
    for (int i = 0; i < RV_RELAY_MAX_SESSIONS; i++) {
        if (st->sessions[i].in_use && st->sessions[i].session_id == session_id) {
            s = &st->sessions[i];
            break;
        }
    }
    if (!s) return;

    for (int i = 0; i < RV_RELAY_MAX_CLIENTS_PER_SESSION; i++) {
        if (!s->clients[i].in_use) continue;
        if (addr_equal(&s->clients[i].addr, from)) continue; // don't echo back
        (void)send_fn(send_ctx, &s->clients[i].addr, pkt, pkt_len);
    }
}
