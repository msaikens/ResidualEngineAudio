#pragma once
#include <stdint.h>

typedef struct rv_opus_enc rv_opus_enc_t;
typedef struct rv_opus_dec rv_opus_dec_t;

typedef struct rv_opus_config {
    int sample_rate;   // 48000
    int channels;      // 1
    int frame_ms;      // 20
    int bitrate_bps;   // 20000 typical start
    int use_fec;       // 0/1
} rv_opus_config_t;

rv_opus_enc_t* rv_opus_enc_create(const rv_opus_config_t* cfg);
void rv_opus_enc_destroy(rv_opus_enc_t* e);

rv_opus_dec_t* rv_opus_dec_create(const rv_opus_config_t* cfg);
void rv_opus_dec_destroy(rv_opus_dec_t* d);

int rv_opus_encode(rv_opus_enc_t* e,
                   const int16_t* pcm,
                   int pcm_samples_per_ch,
                   uint8_t* out,
                   int out_cap);

int rv_opus_decode(rv_opus_dec_t* d,
                   const uint8_t* packet,
                   int packet_len,
                   int16_t* out_pcm,
                   int out_samples_per_ch);
