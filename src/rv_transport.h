#pragma once
#include <stdint.h>
#include "residual_voice/voice.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rv_transport_vtbl {
    // Send one packet. Return bytes sent (>=0) or <0 error.
    int (*send)(void* user, const uint8_t* data, uint32_t len);

    // Recv one packet into out_buf. Return bytes received (>0),
    // 0 if none available, or <0 error.
    int (*recv)(void* user, uint8_t* out_buf, uint32_t cap);
} rv_transport_vtbl_t;

// Pump voice->transport until empty.
// Returns packets sent, or <0 error.
static inline int rv_transport_pump_outgoing(const rv_transport_vtbl_t* t,
                                            void* user,
                                            rv_voice_t* v) {
    if (!t || !t->send || !v) return -1;

    uint8_t pkt[1400];
    uint32_t pkt_len = 0;
    int sent = 0;

    for (;;) {
        int r = rv_voice_poll_outgoing(v, pkt, (uint32_t)sizeof(pkt), &pkt_len);
        if (r == 0) break;
        if (r < 0) return -2;

        int s = t->send(user, pkt, pkt_len);
        if (s < 0) return -3;
        sent++;
    }
    return sent;
}

// Pump transport->voice until no data.
// Returns packets ingested, or <0 error.
static inline int rv_transport_pump_incoming(const rv_transport_vtbl_t* t,
                                            void* user,
                                            rv_voice_t* v,
                                            uint32_t now_ms) {
    if (!t || !t->recv || !v) return -1;

    uint8_t buf[1400];
    int ing = 0;

    for (;;) {
        int r = t->recv(user, buf, (uint32_t)sizeof(buf));
        if (r == 0) break;
        if (r < 0) return -2;

        (void)rv_voice_ingest_packet(v, buf, (uint32_t)r, now_ms);
        ing++;
    }
    return ing;
}

#ifdef __cplusplus
}
#endif
