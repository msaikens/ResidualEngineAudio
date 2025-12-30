#include "residual_voice/voice.h"
#include "rv_event_queue.h"
#include "rv_opus.h"
#include "rv_opus_jitter.h"
#include "rv_udp.h"
#include "rv_netproto.h"
#include <string.h>

#define RV_FRAME_SAMPLES_48K_20MS 960

struct rv_voice {
    rv_voice_config_t cfg;
    rv_voice_allocators_t allocs;
    rv_voice_callbacks_t cbs;

    int connected;

    rv_event_queue_t evq;

    // Opus
    rv_opus_enc_t* enc;
    rv_opus_dec_t* dec;

    // Incoming jitter
    rv_opus_jitter_t jb;

    // Net
    rv_udp_socket_t* udp;
    rv_sockaddr_t relay_addr;
    uint8_t net_buf[1400];

    // Tx
    uint16_t tx_seq;
    uint8_t enc_pkt[RV_OPUS_MAX_PACKET];
    uint8_t pkt_buf[1400];

    // Decode scratch
    int16_t play_pcm[RV_FRAME_SAMPLES_48K_20MS];

    // Timing
    uint32_t last_play_ms;

    // Identity (prototype)
    uint64_t session_id;
    uint16_t player_id;

    // Event backing storage
    char log_buf[256];
    char err_buf[256];
};

static void rv_emit_log(rv_voice_t* v, int level, const char* msg) {
    if (v->cbs.log) v->cbs.log(v->cbs.user, level, msg);

    strncpy(v->log_buf, msg, sizeof(v->log_buf) - 1);
    v->log_buf[sizeof(v->log_buf) - 1] = '\0';

    rv_voice_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = RV_VOICE_EVENT_LOG;
    ev.as.log.level = level;
    ev.as.log.message = v->log_buf;
    (void)rv_eventq_push(&v->evq, &ev);
}

static void rv_emit_error(rv_voice_t* v, rv_voice_result_t code, const char* msg) {
    strncpy(v->err_buf, msg, sizeof(v->err_buf) - 1);
    v->err_buf[sizeof(v->err_buf) - 1] = '\0';

    rv_voice_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = RV_VOICE_EVENT_ERROR;
    ev.as.error.code = code;
    ev.as.error.message = v->err_buf;
    (void)rv_eventq_push(&v->evq, &ev);
}

rv_voice_t* rv_voice_create(const rv_voice_config_t* cfg,
                            const rv_voice_allocators_t* allocs,
                            const rv_voice_callbacks_t* cbs) {
    if (!cfg || cfg->api_version != RV_VOICE_API_VERSION) return NULL;
    if (!allocs || !allocs->alloc || !allocs->free) return NULL;

    rv_voice_t* v = (rv_voice_t*)allocs->alloc(allocs->user, sizeof(rv_voice_t));
    if (!v) return NULL;
    memset(v, 0, sizeof(*v));

    v->cfg = *cfg;
    v->allocs = *allocs;
    if (cbs) v->cbs = *cbs;

    rv_eventq_init(&v->evq);
    rv_opus_jitter_init(&v->jb);

    v->tx_seq = 1;
    v->last_play_ms = 0;

    rv_opus_config_t oc;
    memset(&oc, 0, sizeof(oc));
    oc.sample_rate = (int)(v->cfg.sample_rate_hz ? v->cfg.sample_rate_hz : 48000);
    oc.channels = 1;
    oc.frame_ms = (int)(v->cfg.frame_ms ? v->cfg.frame_ms : 20);
    oc.bitrate_bps = 20000;
    oc.use_fec = 0;

    v->enc = rv_opus_enc_create(&oc);
    v->dec = rv_opus_dec_create(&oc);

    if (!v->enc || !v->dec) {
        rv_emit_error(v, RV_VOICE_ERR_INTERNAL, "Opus init failed (encoder/decoder)");
    } else {
        rv_emit_log(v, 0, "Opus encoder/decoder initialized");
    }

    rv_emit_log(v, 0, "rv_voice_create ok");
    return v;
}

void rv_voice_destroy(rv_voice_t* v) {
    if (!v) return;

    if (v->udp) {
        rv_udp_destroy(v->udp);
        v->udp = NULL;
        rv_udp_cleanup();
    }

    rv_opus_enc_destroy(v->enc);
    rv_opus_dec_destroy(v->dec);
    v->allocs.free(v->allocs.user, v);
}

rv_voice_result_t rv_voice_connect(rv_voice_t* v, const rv_voice_connect_info_t* info) {


    if (!v || !info || !info->relay_host) return RV_VOICE_ERR_INVALID_ARGUMENT;

    v->session_id = info->session_id;
    v->player_id = info->player_id ? info->player_id : 1;
 // default; demo can override later if you extend connect_info

    if (rv_udp_startup() != 0) {
        rv_emit_error(v, RV_VOICE_ERR_INTERNAL, "WSAStartup failed");
        return RV_VOICE_ERR_INTERNAL;
    }

    uint16_t bind_port = info->local_port; // 0 => ephemeral
    v->udp = rv_udp_create_dualstack(bind_port, 1 /*nonblocking*/);
    if (!v->udp) {
        rv_udp_cleanup();
        rv_emit_error(v, RV_VOICE_ERR_INTERNAL, "udp socket create/bind failed");
        return RV_VOICE_ERR_INTERNAL;
    }

    if (rv_parse_ip_port(info->relay_host, info->relay_port, &v->relay_addr) != 0) {
        rv_emit_error(v, RV_VOICE_ERR_INVALID_ARGUMENT,
                      "relay_host must be IPv4/IPv6 literal for now (e.g. 127.0.0.1 or ::1)");
        return RV_VOICE_ERR_INVALID_ARGUMENT;
    }

    // Send JOIN
    int join_len = rv_build_join_packet(v->pkt_buf, (int)sizeof(v->pkt_buf), v->session_id, v->player_id);
    if (join_len > 0) {
    (void)rv_udp_sendto(v->udp, &v->relay_addr, v->pkt_buf, join_len);
    (void)rv_udp_sendto(v->udp, &v->relay_addr, v->pkt_buf, join_len);
    (void)rv_udp_sendto(v->udp, &v->relay_addr, v->pkt_buf, join_len);
}

    v->connected = 1;

    rv_voice_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = RV_VOICE_EVENT_CONNECTED;
    (void)rv_eventq_push(&v->evq, &ev);

    rv_emit_log(v, 0, "rv_voice_connect (udp+opus) connected");
    return RV_VOICE_OK;
}

rv_voice_result_t rv_voice_disconnect(rv_voice_t* v) {
    if (!v) return RV_VOICE_ERR_INVALID_ARGUMENT;

    v->connected = 0;

    if (v->udp) {
        rv_udp_destroy(v->udp);
        v->udp = NULL;
        rv_udp_cleanup();
    }

    rv_voice_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = RV_VOICE_EVENT_DISCONNECTED;
    (void)rv_eventq_push(&v->evq, &ev);

    rv_emit_log(v, 0, "rv_voice_disconnect disconnected");
    return RV_VOICE_OK;
}

rv_voice_result_t rv_voice_submit_capture_pcm(rv_voice_t* v,
                                              const int16_t* samples,
                                              uint32_t sample_count) {
    if (!v || !samples) return RV_VOICE_ERR_INVALID_ARGUMENT;
    if (!v->connected) return RV_VOICE_ERR_NOT_CONNECTED;
    if (!v->udp) return RV_VOICE_ERR_INTERNAL;

    if (sample_count != RV_FRAME_SAMPLES_48K_20MS) {
        rv_emit_error(v, RV_VOICE_ERR_INVALID_ARGUMENT, "capture_pcm: expected 960 samples (20ms@48k mono)");
        return RV_VOICE_ERR_INVALID_ARGUMENT;
    }

    int bytes = rv_opus_encode(v->enc, samples, (int)sample_count, v->enc_pkt, (int)sizeof(v->enc_pkt));
    if (bytes < 0) {
        rv_emit_error(v, RV_VOICE_ERR_INTERNAL, "opus_encode failed");
        return RV_VOICE_ERR_INTERNAL;
    }

    int pkt_len = rv_build_voice_packet(v->pkt_buf, (int)sizeof(v->pkt_buf),
                                        v->player_id, v->tx_seq++,
                                        v->enc_pkt, (uint16_t)bytes);
    if (pkt_len < 0) {
        rv_emit_error(v, RV_VOICE_ERR_INTERNAL, "build_voice_packet failed");
        return RV_VOICE_ERR_INTERNAL;
    }

    (void)rv_udp_sendto(v->udp, &v->relay_addr, v->pkt_buf, pkt_len);
    return RV_VOICE_OK;
}

rv_voice_result_t rv_voice_set_local_state(rv_voice_t* v, const rv_voice_player_state_t* st) {
    (void)v; (void)st;
    return RV_VOICE_OK;
}

rv_voice_result_t rv_voice_tick(rv_voice_t* v, uint32_t now_ms) {
    if (!v) return RV_VOICE_ERR_INVALID_ARGUMENT;
    if (!v->connected) return RV_VOICE_OK;

    // recv
    if (v->udp) {
        for (;;) {
            rv_sockaddr_t from;
            int r = rv_udp_recvfrom(v->udp, &from, v->net_buf, (int)sizeof(v->net_buf));
            if (r <= 0) break;

            rv_pkt_hdr_t hdr;
            if (rv_parse_packet_header(v->net_buf, r, &hdr) != 0) continue;

            if (hdr.type == RV_PKT_VOICE) {
                uint16_t speaker_id = 0, seq = 0, plen = 0;
                const uint8_t* payload = NULL;
                if (rv_parse_voice_packet(v->net_buf, r, &speaker_id, &seq, &payload, &plen) == 0) {
                    // ignore echo from self if server misconfigured
                    if (speaker_id == v->player_id) continue;
                    rv_opus_jitter_push(&v->jb, seq, payload, plen);
                }
            }
        }
    }

    // playback tick
    const uint32_t frame_ms = (v->cfg.frame_ms ? v->cfg.frame_ms : 20);
    if (v->last_play_ms == 0) v->last_play_ms = now_ms;

    while ((uint32_t)(now_ms - v->last_play_ms) >= frame_ms) {
        v->last_play_ms += frame_ms;

        const uint8_t* pkt = NULL;
        uint16_t pkt_len = 0;

        if (rv_opus_jitter_pop(&v->jb, &pkt, &pkt_len)) {
            int n = rv_opus_decode(v->dec, pkt, (int)pkt_len, v->play_pcm, RV_FRAME_SAMPLES_48K_20MS);
            if (n < 0) {
                rv_emit_error(v, RV_VOICE_ERR_INTERNAL, "opus_decode failed");
            } else {
                rv_voice_event_t ev;
                memset(&ev, 0, sizeof(ev));
                ev.type = RV_VOICE_EVENT_PCM_FRAME;
                ev.as.pcm.speaker_id = 999; // remote (prototype)
                ev.as.pcm.sample_rate = v->cfg.sample_rate_hz ? v->cfg.sample_rate_hz : 48000;
                ev.as.pcm.channels = 1;
                ev.as.pcm.samples = v->play_pcm;
                ev.as.pcm.sample_count = (uint32_t)n;
                (void)rv_eventq_push(&v->evq, &ev);
            }
        }
    }

    return RV_VOICE_OK;
}

int rv_voice_poll_event(rv_voice_t* v, rv_voice_event_t* out_event) {
    if (!v || !out_event) return 0;
    return rv_eventq_pop(&v->evq, out_event);
}
