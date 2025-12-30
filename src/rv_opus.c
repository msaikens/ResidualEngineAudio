#include "rv_opus.h"
#include <opus/opus.h>
#include <stdlib.h>
#include <string.h>

struct rv_opus_enc {
    OpusEncoder* enc;
    rv_opus_config_t cfg;
    int frame_samples;
};

struct rv_opus_dec {
    OpusDecoder* dec;
    rv_opus_config_t cfg;
    int frame_samples;
};

static int frame_samples(int sample_rate, int frame_ms) {
    return (sample_rate / 1000) * frame_ms;
}

rv_opus_enc_t* rv_opus_enc_create(const rv_opus_config_t* cfg) {
    if (!cfg) return NULL;
    int err = 0;

    rv_opus_enc_t* e = (rv_opus_enc_t*)malloc(sizeof(*e));
    if (!e) return NULL;
    memset(e, 0, sizeof(*e));

    e->cfg = *cfg;
    e->frame_samples = frame_samples(cfg->sample_rate, cfg->frame_ms);

    e->enc = opus_encoder_create(cfg->sample_rate, cfg->channels, OPUS_APPLICATION_VOIP, &err);
    if (err != OPUS_OK || !e->enc) { free(e); return NULL; }

    opus_encoder_ctl(e->enc, OPUS_SET_BITRATE(cfg->bitrate_bps));
    opus_encoder_ctl(e->enc, OPUS_SET_INBAND_FEC(cfg->use_fec ? 1 : 0));
    opus_encoder_ctl(e->enc, OPUS_SET_PACKET_LOSS_PERC(5));

    return e;
}

void rv_opus_enc_destroy(rv_opus_enc_t* e) {
    if (!e) return;
    if (e->enc) opus_encoder_destroy(e->enc);
    free(e);
}

rv_opus_dec_t* rv_opus_dec_create(const rv_opus_config_t* cfg) {
    if (!cfg) return NULL;
    int err = 0;

    rv_opus_dec_t* d = (rv_opus_dec_t*)malloc(sizeof(*d));
    if (!d) return NULL;
    memset(d, 0, sizeof(*d));

    d->cfg = *cfg;
    d->frame_samples = frame_samples(cfg->sample_rate, cfg->frame_ms);

    d->dec = opus_decoder_create(cfg->sample_rate, cfg->channels, &err);
    if (err != OPUS_OK || !d->dec) { free(d); return NULL; }

    return d;
}

void rv_opus_dec_destroy(rv_opus_dec_t* d) {
    if (!d) return;
    if (d->dec) opus_decoder_destroy(d->dec);
    free(d);
}

int rv_opus_encode(rv_opus_enc_t* e,
                   const int16_t* pcm,
                   int pcm_samples_per_ch,
                   uint8_t* out,
                   int out_cap) {
    if (!e || !pcm || !out) return -1;
    if (pcm_samples_per_ch != e->frame_samples) return -2;
    return opus_encode(e->enc, pcm, pcm_samples_per_ch, out, out_cap);
}

int rv_opus_decode(rv_opus_dec_t* d,
                   const uint8_t* packet,
                   int packet_len,
                   int16_t* out_pcm,
                   int out_samples_per_ch) {
    if (!d || !out_pcm) return -1;
    if (out_samples_per_ch < d->frame_samples) return -2;

    // packet NULL or len=0 => PLC
    int fec = 0;
    return opus_decode(d->dec,
                       packet,
                       packet ? packet_len : 0,
                       out_pcm,
                       d->frame_samples,
                       fec);
}
