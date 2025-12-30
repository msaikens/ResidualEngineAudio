#pragma once
#include <stdint.h>

#ifndef RV_OPUS_JITTER_CAP
#define RV_OPUS_JITTER_CAP 128
#endif

#ifndef RV_OPUS_MAX_PACKET
#define RV_OPUS_MAX_PACKET 600
#endif

typedef struct rv_opus_packet {
    uint16_t seq;
    uint16_t len;
    uint8_t  valid;
    uint8_t  data[RV_OPUS_MAX_PACKET];
} rv_opus_packet_t;

typedef struct rv_opus_jitter {
    rv_opus_packet_t packets[RV_OPUS_JITTER_CAP];
    uint16_t next_play_seq;
    uint8_t started;
} rv_opus_jitter_t;

void rv_opus_jitter_init(rv_opus_jitter_t* jb);
void rv_opus_jitter_push(rv_opus_jitter_t* jb, uint16_t seq, const uint8_t* data, uint16_t len);

// out_data NULL + out_len 0 indicates PLC
int rv_opus_jitter_pop(rv_opus_jitter_t* jb, const uint8_t** out_data, uint16_t* out_len);
