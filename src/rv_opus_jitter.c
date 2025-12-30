#include "rv_opus_jitter.h"
#include <string.h>

static uint32_t slot_for_seq(uint16_t seq) {
    return (uint32_t)(seq % RV_OPUS_JITTER_CAP);
}

void rv_opus_jitter_init(rv_opus_jitter_t* jb) {
    memset(jb, 0, sizeof(*jb));
}

void rv_opus_jitter_push(rv_opus_jitter_t* jb, uint16_t seq, const uint8_t* data, uint16_t len) {
    if (len == 0 || len > RV_OPUS_MAX_PACKET) return;

    uint32_t slot = slot_for_seq(seq);
    jb->packets[slot].seq = seq;
    jb->packets[slot].len = len;
    jb->packets[slot].valid = 1;
    memcpy(jb->packets[slot].data, data, len);

    if (!jb->started) {
        jb->started = 1;
        jb->next_play_seq = seq;
    }
}

int rv_opus_jitter_pop(rv_opus_jitter_t* jb, const uint8_t** out_data, uint16_t* out_len) {
    if (!jb->started) return 0;

    uint16_t want = jb->next_play_seq;
    uint32_t slot = slot_for_seq(want);

    if (jb->packets[slot].valid && jb->packets[slot].seq == want) {
        *out_data = jb->packets[slot].data;
        *out_len  = jb->packets[slot].len;
        jb->packets[slot].valid = 0;
    } else {
        *out_data = NULL;
        *out_len = 0; // PLC
    }

    jb->next_play_seq++;
    return 1;
}
