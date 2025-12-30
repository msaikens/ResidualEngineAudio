#include "rv_netproto.h"
#include <string.h>

static void rv_hdr_init(rv_pkt_hdr_t* h, uint8_t type, uint16_t speaker_id, uint16_t seq, uint16_t payload_len) {
    memset(h, 0, sizeof(*h));
    h->magic = rv_htonl32(RV_MAGIC);
    h->version = RV_PROTO_VER;
    h->type = type;
    h->flags = 0;
    h->speaker_id = rv_htons16(speaker_id);
    h->seq = rv_htons16(seq);
    h->payload_len = rv_htons16(payload_len);
}

int rv_build_join_packet(uint8_t* out, int out_cap, uint64_t session_id, uint16_t player_id) {
    if (!out) return -1;
    const uint16_t payload_len = (uint16_t)sizeof(rv_join_payload_t);
    const int need = (int)sizeof(rv_pkt_hdr_t) + (int)payload_len;
    if (out_cap < need) return -2;

    rv_pkt_hdr_t h;
    rv_hdr_init(&h, RV_PKT_JOIN, 0, 0, payload_len);

    rv_join_payload_t p;
    memset(&p, 0, sizeof(p));
    p.session_id = rv_htonll64(session_id);
    p.player_id = rv_htons16(player_id);
    p.reserved = 0;

    memcpy(out, &h, sizeof(h));
    memcpy(out + sizeof(h), &p, sizeof(p));
    return need;
}

int rv_build_voice_packet(uint8_t* out, int out_cap,
                          uint16_t speaker_id, uint16_t seq,
                          const uint8_t* payload, uint16_t payload_len) {
    if (!out || !payload) return -1;
    if (payload_len == 0) return -2;

    const int need = (int)sizeof(rv_pkt_hdr_t) + (int)payload_len;
    if (out_cap < need) return -3;

    rv_pkt_hdr_t h;
    rv_hdr_init(&h, RV_PKT_VOICE, speaker_id, seq, payload_len);

    memcpy(out, &h, sizeof(h));
    memcpy(out + sizeof(h), payload, payload_len);
    return need;
}

int rv_parse_packet_header(const uint8_t* buf, int len, rv_pkt_hdr_t* out_hdr_host) {
    if (!buf || len < (int)sizeof(rv_pkt_hdr_t) || !out_hdr_host) return -1;

    rv_pkt_hdr_t h;
    memcpy(&h, buf, sizeof(h));

    if (rv_ntohl32(h.magic) != RV_MAGIC) return -2;
    if (h.version != RV_PROTO_VER) return -3;

    // Convert to host order in the output
    out_hdr_host->magic = RV_MAGIC;
    out_hdr_host->version = h.version;
    out_hdr_host->type = h.type;
    out_hdr_host->flags = h.flags;
    out_hdr_host->reserved0 = 0;
    out_hdr_host->speaker_id = rv_ntohs16(h.speaker_id);
    out_hdr_host->seq = rv_ntohs16(h.seq);
    out_hdr_host->payload_len = rv_ntohs16(h.payload_len);

    const int need = (int)sizeof(rv_pkt_hdr_t) + (int)out_hdr_host->payload_len;
    if (out_hdr_host->payload_len == 0 || len < need) return -4;

    return 0;
}

int rv_parse_join_payload(const uint8_t* buf, int len, uint64_t* out_session_id, uint16_t* out_player_id) {
    rv_pkt_hdr_t h;
    int r = rv_parse_packet_header(buf, len, &h);
    if (r != 0) return r;
    if (h.type != RV_PKT_JOIN) return -10;
    if (h.payload_len != sizeof(rv_join_payload_t)) return -11;

    rv_join_payload_t p;
    memcpy(&p, buf + sizeof(rv_pkt_hdr_t), sizeof(p));

    if (out_session_id) *out_session_id = rv_ntohll64(p.session_id);
    if (out_player_id) *out_player_id = rv_ntohs16(p.player_id);
    return 0;
}

int rv_parse_voice_packet(const uint8_t* buf, int len,
                          uint16_t* out_speaker_id, uint16_t* out_seq,
                          const uint8_t** out_payload, uint16_t* out_payload_len) {
    rv_pkt_hdr_t h;
    int r = rv_parse_packet_header(buf, len, &h);
    if (r != 0) return r;
    if (h.type != RV_PKT_VOICE) return -20;

    if (out_speaker_id) *out_speaker_id = h.speaker_id;
    if (out_seq) *out_seq = h.seq;
    if (out_payload_len) *out_payload_len = h.payload_len;
    if (out_payload) *out_payload = buf + sizeof(rv_pkt_hdr_t);

    return 0;
}
