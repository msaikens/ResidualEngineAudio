#pragma once
#include <stdint.h>
#include "residual_voice/voice.h"

#ifdef __cplusplus
extern "C" {
#endif

// A transport shim is just two function pointers.
// - send: deliver one datagram (or packet) to your relay/peer
// - recv: fetch one datagram into out buffer
//
// Return conventions:
// send: >=0 bytes sent, <0 error
// recv:  >0 bytes received, 0 = no data available, <0 error
typedef struct rv_shim_transport {
    void* user;

    int (*send)(void* user, const uint8_t* data, uint32_t len);
    int (*recv)(void* user, uint8_t* out, uint32_t cap);
} rv_shim_transport_t;

// Pump all outgoing voice packets into the transport.
// Returns number of packets sent, or <0 on error.
int rv_shim_pump_outgoing(const rv_shim_transport_t* t, rv_voice_t* v);

// Pump all incoming transport packets into the voice core.
// Returns number of packets ingested, or <0 on error.
int rv_shim_pump_incoming(const rv_shim_transport_t* t, rv_voice_t* v, uint32_t now_ms);

#ifdef __cplusplus
}
#endif
