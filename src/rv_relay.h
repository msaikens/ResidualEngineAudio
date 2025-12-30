#pragma once
#include <stdint.h>
#include "rv_udp.h"

#define RV_RELAY_MAX_SESSIONS 64
#define RV_RELAY_MAX_CLIENTS_PER_SESSION 16

typedef struct rv_relay_client {
    uint8_t in_use;
    uint16_t player_id;
    rv_sockaddr_t addr;
} rv_relay_client_t;

typedef struct rv_relay_session {
    uint8_t in_use;
    uint64_t session_id;
    rv_relay_client_t clients[RV_RELAY_MAX_CLIENTS_PER_SESSION];
} rv_relay_session_t;

typedef struct rv_relay_state {
    rv_relay_session_t sessions[RV_RELAY_MAX_SESSIONS];
} rv_relay_state_t;

void rv_relay_init(rv_relay_state_t* st);

// Register/update client endpoint in session
void rv_relay_join(rv_relay_state_t* st, uint64_t session_id, uint16_t player_id, const rv_sockaddr_t* from);

// Forward a received VOICE packet to all other clients in same session
void rv_relay_forward_voice(rv_relay_state_t* st,
                            uint64_t session_id,
                            const rv_sockaddr_t* from,
                            const uint8_t* pkt,
                            int pkt_len,
                            void* send_ctx,
                            int (*send_fn)(void* ctx, const rv_sockaddr_t* to, const uint8_t* data, int len));
