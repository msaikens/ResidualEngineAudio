#include "rv_shim_transport.h"

int rv_shim_pump_outgoing(const rv_shim_transport_t* t, rv_voice_t* v) {
    if (!t || !t->send || !v) return -1;

    uint8_t pkt[1400];
    uint32_t pkt_len = 0;
    int sent = 0;

    for (;;) {
        int r = rv_voice_poll_outgoing(v, pkt, (uint32_t)sizeof(pkt), &pkt_len);
        if (r == 0) break;     // no packets
        if (r < 0) return -2;  // voice error / buffer too small

        int s = t->send(t->user, pkt, pkt_len);
        if (s < 0) return -3;  // transport error

        sent++;
    }

    return sent;
}

int rv_shim_pump_incoming(const rv_shim_transport_t* t, rv_voice_t* v, uint32_t now_ms) {
    if (!t || !t->recv || !v) return -1;

    uint8_t buf[1400];
    int ing = 0;

    for (;;) {
        int r = t->recv(t->user, buf, (uint32_t)sizeof(buf));
        if (r == 0) break;     // no data (nonblocking)
        if (r < 0) return -2;  // transport error

        (void)rv_voice_ingest_packet(v, buf, (uint32_t)r, now_ms);
        ing++;
    }

    return ing;
}
