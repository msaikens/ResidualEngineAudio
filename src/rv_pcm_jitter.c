#include "rv_pcm_jitter.h"
#include <string.h>

static uint32_t slot_for_seq(uint16_t seq) {
    return (uint32_t)(seq % RV_PCM_JITTER_CAP);
}

void rv_pcm_jitter_init(rv_pcm_jitter_t* jb) {
    memset(jb, 0, sizeof(*jb));
}

void rv_pcm_jitter_push(rv_pcm_jitter_t* jb, uint16_t seq, const int16_t* pcm) {
    uint32_t slot = slot_for_seq(seq);
    jb->frames[slot].seq = seq;
    jb->frames[slot].valid = 1;
    memcpy(jb->frames[slot].pcm, pcm, sizeof(int16_t) * RV_PCM_FRAME_SAMPLES);

    if (!jb->started) {
        jb->started = 1;
        jb->next_play_seq = seq;
        jb->highest_seq_seen = seq;
    } else {
        // track highest seen (wrap-safe enough for short sessions)
        if ((uint16_t)(seq - jb->highest_seq_seen) < 0x8000) {
            jb->highest_seq_seen = seq;
        }
    }
}

int rv_pcm_jitter_pop(rv_pcm_jitter_t* jb, int16_t* out_pcm) {
    if (!jb->started) return 0;

    uint16_t want = jb->next_play_seq;
    uint32_t slot = slot_for_seq(want);

    if (jb->frames[slot].valid && jb->frames[slot].seq == want) {
        memcpy(out_pcm, jb->frames[slot].pcm, sizeof(int16_t) * RV_PCM_FRAME_SAMPLES);
        jb->frames[slot].valid = 0;
    } else {
        // missing frame: output silence for Phase 1
        memset(out_pcm, 0, sizeof(int16_t) * RV_PCM_FRAME_SAMPLES);
    }

    jb->next_play_seq++;
    return 1;
}
