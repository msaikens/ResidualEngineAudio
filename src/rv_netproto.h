#pragma once
#include <stdint.h>

#define RV_MAGIC 0x43565652u /* 'RVVC' little-endian */
#define RV_PROTO_VER 1

typedef enum rv_pkt_type {
    RV_PKT_JOIN  = 1,
    RV_PKT_VOICE = 2,
} rv_pkt_type_t;

#pragma pack(push, 1)
typedef struct rv_pkt_hdr {
    uint32_t magic;       // RV_MAGIC (network order)
    uint8_t  version;     // RV_PROTO_VER
    uint8_t  type;        // rv_pkt_type_t
    uint8_t  flags;       // reserved (ptt/radio later)
    uint8_t  reserved0;
    uint16_t speaker_id;  // network order (VOICE); for JOIN may be 0
    uint16_t seq;         // network order (VOICE); for JOIN 0
    uint16_t payload_len; // network order
} rv_pkt_hdr_t;

typedef struct rv_join_payload {
    uint64_t session_id;  // network order (u64 big-endian)
    uint16_t player_id;   // network order (1..16)
    uint16_t reserved;    // 0
} rv_join_payload_t;
#pragma pack(pop)

static inline uint16_t rv_bswap16(uint16_t x) { return (uint16_t)((x << 8) | (x >> 8)); }
static inline uint32_t rv_bswap32(uint32_t x) {
    return ((x & 0x000000FFu) << 24) |
           ((x & 0x0000FF00u) << 8)  |
           ((x & 0x00FF0000u) >> 8)  |
           ((x & 0xFF000000u) >> 24);
}
static inline uint64_t rv_bswap64(uint64_t x) {
    return ((x & 0x00000000000000FFull) << 56) |
           ((x & 0x000000000000FF00ull) << 40) |
           ((x & 0x0000000000FF0000ull) << 24) |
           ((x & 0x00000000FF000000ull) << 8)  |
           ((x & 0x000000FF00000000ull) >> 8)  |
           ((x & 0x0000FF0000000000ull) >> 24) |
           ((x & 0x00FF000000000000ull) >> 40) |
           ((x & 0xFF00000000000000ull) >> 56);
}

static inline uint16_t rv_htons16(uint16_t x) { return rv_bswap16(x); }
static inline uint16_t rv_ntohs16(uint16_t x) { return rv_bswap16(x); }
static inline uint32_t rv_htonl32(uint32_t x) { return rv_bswap32(x); }
static inline uint32_t rv_ntohl32(uint32_t x) { return rv_bswap32(x); }
static inline uint64_t rv_htonll64(uint64_t x) { return rv_bswap64(x); }
static inline uint64_t rv_ntohll64(uint64_t x) { return rv_bswap64(x); }

int rv_build_join_packet(uint8_t* out, int out_cap, uint64_t session_id, uint16_t player_id);

int rv_build_voice_packet(uint8_t* out, int out_cap,
                          uint16_t speaker_id, uint16_t seq,
                          const uint8_t* payload, uint16_t payload_len);

int rv_parse_packet_header(const uint8_t* buf, int len, rv_pkt_hdr_t* out_hdr_host);

// JOIN parser
int rv_parse_join_payload(const uint8_t* buf, int len, uint64_t* out_session_id, uint16_t* out_player_id);

// VOICE parser
int rv_parse_voice_packet(const uint8_t* buf, int len,
                          uint16_t* out_speaker_id, uint16_t* out_seq,
                          const uint8_t** out_payload, uint16_t* out_payload_len);
