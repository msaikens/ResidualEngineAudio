#pragma once
#include <stdint.h>

#ifndef RV_PCM_FRAME_SAMPLES
// 20ms @ 48kHz = 960 samples
#define RV_PCM_FRAME_SAMPLES 960
#endif

#ifndef RV_PCM_JITTER_CAP
#define RV_PCM_JITTER_CAP 64
#endif

typedef struct rv_pcm_frame {
    uint16_t seq;
    uint8_t  valid;
    int16_t  pcm[RV_PCM_FRAME_SAMPLES]; // mono for Phase 1
} rv_pcm_frame_t;

typedef struct rv_pcm_jitter {
    rv_pcm_frame_t frames[RV_PCM_JITTER_CAP];
    uint16_t next_play_seq;
    uint16_t highest_seq_seen;
    uint8_t  started;
} rv_pcm_jitter_t;

void rv_pcm_jitter_init(rv_pcm_jitter_t* jb);
void rv_pcm_jitter_push(rv_pcm_jitter_t* jb, uint16_t seq, const int16_t* pcm);
int  rv_pcm_jitter_pop(rv_pcm_jitter_t* jb, int16_t* out_pcm);
