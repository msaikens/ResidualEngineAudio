#pragma once
#include <stdint.h>

#include "rv_udp.h"

#include "residual_voice/voice.h"  // <-- ensures rv_voice_t is known here

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rv_transport_udp rv_transport_udp_t;

typedef struct rv_transport_udp_config {
    const char* relay_host;   // literal IP
    uint16_t    relay_port;
    uint16_t    local_port;   // 0 => bind to relay_port
    int         nonblocking;  // 1 recommended
} rv_transport_udp_config_t;

rv_transport_udp_t* rv_transport_udp_create(const rv_transport_udp_config_t* cfg);
void rv_transport_udp_destroy(rv_transport_udp_t* t);

int rv_transport_udp_sendto_relay(rv_transport_udp_t* t, const uint8_t* data, uint32_t len);
int rv_transport_udp_recv(rv_transport_udp_t* t, uint8_t* out, uint32_t cap, uint32_t* out_len);

// Pump *all* outgoing packets from voice -> UDP relay.
int rv_transport_udp_pump_voice_outgoing(rv_transport_udp_t* t, rv_voice_t* v);

// Pump UDP -> voice by ingesting all available packets.
int rv_transport_udp_pump_voice_incoming(rv_transport_udp_t* t, rv_voice_t* v, uint32_t now_ms);

#ifdef __cplusplus
}
#endif
