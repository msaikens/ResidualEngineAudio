// src/voice.c
#include "residual_voice/voice.h"

#include "rv_opus.h"
#include "rv_opus_jitter.h"
#include "rv_netproto.h"
#include "rv_event_queue.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdatomic.h>

#ifndef RV_CAPTURE_MAX_SAMPLES
#define RV_CAPTURE_MAX_SAMPLES 2880u
#endif

#ifndef RV_CAPTURE_RING_CAP
#define RV_CAPTURE_RING_CAP 16u
#endif

#ifndef RV_MAX_OUT_PKTS
#define RV_MAX_OUT_PKTS 256u
#endif

#ifndef RV_MAX_PKT_SIZE
#define RV_MAX_PKT_SIZE 1400u
#endif

/* ============================================================
   Internal packet queue (engine -> host transport)
   ============================================================ */

typedef struct rv_out_pkt {
    uint16_t len;
    uint8_t  data[RV_MAX_PKT_SIZE];
} rv_out_pkt_t;

/* ============================================================
   SPSC capture queue (audio thread -> voice thread)
   ============================================================ */

typedef struct rv_pcm_frame {
    uint32_t count; // samples per channel
    int16_t  samples[RV_CAPTURE_MAX_SAMPLES];
} rv_pcm_frame_t;

typedef struct rv_spsc_pcm_ring {
    _Atomic uint32_t w;
    _Atomic uint32_t r;
    rv_pcm_frame_t slots[RV_CAPTURE_RING_CAP];
} rv_spsc_pcm_ring_t;

static inline void rv_ring_init(rv_spsc_pcm_ring_t* q) {
    atomic_store_explicit(&q->w, 0u, memory_order_relaxed);
    atomic_store_explicit(&q->r, 0u, memory_order_relaxed);
}

static inline int rv_ring_push(rv_spsc_pcm_ring_t* q, const int16_t* samples, uint32_t n) {
    uint32_t w = atomic_load_explicit(&q->w, memory_order_relaxed);
    uint32_t r = atomic_load_explicit(&q->r, memory_order_acquire);

    if (w - r >= RV_CAPTURE_RING_CAP) return 0; // full
    if (n == 0 || n > RV_CAPTURE_MAX_SAMPLES) return 0;

    rv_pcm_frame_t* slot = &q->slots[w % RV_CAPTURE_RING_CAP];
    slot->count = n;
    memcpy(slot->samples, samples, n * sizeof(int16_t));

    atomic_store_explicit(&q->w, w + 1u, memory_order_release);
    return 1;
}

static inline int rv_ring_pop(rv_spsc_pcm_ring_t* q, rv_pcm_frame_t* out) {
    uint32_t r = atomic_load_explicit(&q->r, memory_order_relaxed);
    uint32_t w = atomic_load_explicit(&q->w, memory_order_acquire);

    if (r == w) return 0; // empty

    const rv_pcm_frame_t* slot = &q->slots[r % RV_CAPTURE_RING_CAP];
    *out = *slot;

    atomic_store_explicit(&q->r, r + 1u, memory_order_release);
    return 1;
}

/* ============================================================
   Version helpers (voice.h defines MAJOR/MINOR macros)
   ============================================================ */

static inline uint32_t rv_ver_major(uint32_t v) { return v >> 16; }
static inline uint32_t rv_ver_minor(uint32_t v) { return v & 0xFFFFu; }

/* ============================================================
   Core state
   ============================================================ */

struct rv_voice {
    rv_voice_config_t cfg;
    rv_voice_allocators_t allocs;
    rv_voice_callbacks_t cbs;

    int initialized;
    int connected;

    uint64_t session_id;
    uint16_t player_id;

    rv_opus_config_t opus_cfg;

    rv_opus_enc_t* enc;
    rv_opus_dec_t** dec;         // [max_players]
    rv_opus_jitter_t* jb;        // [max_players]

    int16_t**  pcm_buf;          // [max_players] decoded frame per speaker
    uint32_t*  pcm_count;        // [max_players] decoded samples per frame
    uint32_t   frame_samples;    // samples per channel per frame

    // Thread-safe capture queue (audio thread -> voice thread)
    rv_spsc_pcm_ring_t cap_q;

    uint8_t*   speaking;         // [max_players]
    uint32_t*  last_rx_ms;        // [max_players]

    // Remember latest rx flags per speaker so host can route PCM events
    uint8_t*   last_rx_flags;     // [max_players]

    // Local state (PTT, radio mode, channel)
    rv_voice_player_state_t local_state;
    int has_local_state;

    // Outgoing network packets (voice -> transport)
    rv_out_pkt_t out_q[RV_MAX_OUT_PKTS];
    uint32_t out_r;
    uint32_t out_w;

    uint16_t seq;

    // Events
    rv_event_queue_t evq;

    // Two alternating message buffers for events/callbacks (safe across poll boundaries)
    char msg_buf[2][256];
    uint32_t msg_flip;
};

/* ============================================================
   Alloc helpers
   ============================================================ */

static void* rv_alloc_raw(const rv_voice_allocators_t* a, size_t sz) {
    if (a && a->alloc) return a->alloc(a->user, sz);
    return malloc(sz);
}

static void rv_free_raw(const rv_voice_allocators_t* a, void* p) {
    if (!p) return;
    if (a && a->free) a->free(a->user, p);
    else free(p);
}

static void* rv_alloc_mem(rv_voice_t* v, size_t sz) {
    return rv_alloc_raw(v ? &v->allocs : NULL, sz);
}

static void rv_free_mem(rv_voice_t* v, void* p) {
    rv_free_raw(v ? &v->allocs : NULL, p);
}

static char* rv_next_msg_buf(rv_voice_t* v) {
    char* buf = v->msg_buf[v->msg_flip & 1u];
    v->msg_flip++;
    return buf;
}

static void rv_emit_log(rv_voice_t* v, int level, const char* msg) {
    if (!v) return;

    if (v->cbs.log) v->cbs.log(v->cbs.user, level, msg ? msg : "");

    rv_voice_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = RV_VOICE_EVENT_LOG;
    ev.as.log.level = level;

    char* dst = rv_next_msg_buf(v);
    strncpy(dst, msg ? msg : "", 255);
    dst[255] = 0;
    ev.as.log.message = dst;

    (void)rv_eventq_push(&v->evq, &ev);
}

static void rv_emit_error(rv_voice_t* v, rv_voice_result_t code, const char* msg) {
    if (!v) return;

    rv_voice_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = RV_VOICE_EVENT_ERROR;
    ev.as.error.code = code;

    char* dst = rv_next_msg_buf(v);
    strncpy(dst, msg ? msg : "", 255);
    dst[255] = 0;
    ev.as.error.message = dst;

    (void)rv_eventq_push(&v->evq, &ev);
}

/* ============================================================
   Outgoing packet queue helpers
   ============================================================ */

static inline uint32_t out_count(const rv_voice_t* v) { return v->out_w - v->out_r; }

static int out_push(rv_voice_t* v, const uint8_t* data, uint32_t len) {
    if (!v || !data || len == 0) return 0;
    if (len > RV_MAX_PKT_SIZE) return 0;
    if (out_count(v) >= RV_MAX_OUT_PKTS) return 0;

    rv_out_pkt_t* p = &v->out_q[v->out_w % RV_MAX_OUT_PKTS];
    p->len = (uint16_t)len;
    memcpy(p->data, data, len);
    v->out_w++;
    return 1;
}

static int out_pop(rv_voice_t* v, uint8_t* out, uint32_t cap, uint32_t* out_len) {
    if (!v || !out || !out_len) return -1;
    if (v->out_r == v->out_w) return 0;

    rv_out_pkt_t* p = &v->out_q[v->out_r % RV_MAX_OUT_PKTS];
    if ((uint32_t)p->len > cap) return -1;

    memcpy(out, p->data, p->len);
    *out_len = p->len;
    v->out_r++;
    return 1;
}

/* ============================================================
   Routing / capture policy helpers
   ============================================================ */

static inline void rv_get_tx_flags(const rv_voice_t* v,
                                  uint8_t* out_is_radio,
                                  uint8_t* out_ch,
                                  uint8_t* out_ptt,
                                  uint8_t* out_flags)
{
    uint8_t is_radio = 0, ch = 0, ptt = 0;

    if (v->has_local_state) {
        ptt = v->local_state.ptt_down ? 1u : 0u;
        is_radio = v->local_state.radio_enabled ? 1u : 0u;
        ch = (uint8_t)(v->local_state.radio_channel & 0x0Fu);
    }

    if (out_is_radio) *out_is_radio = is_radio;
    if (out_ch) *out_ch = ch;
    if (out_ptt) *out_ptt = ptt;
    if (out_flags) *out_flags = rv_flags_make(is_radio, ch, ptt);
}

static inline int rv_capture_should_transmit(const rv_voice_t* v) {
    // PTT_ONLY: require ptt_down for everything
    if (v->cfg.capture_mode == RV_VOICE_CAPTURE_PTT_ONLY) {
        return (v->has_local_state && v->local_state.ptt_down) ? 1 : 0;
    }

    // ALWAYS_ON:
    // - proximity always allowed
    // - radio still requires PTT
    if (v->has_local_state && v->local_state.radio_enabled) {
        return (v->local_state.ptt_down != 0) ? 1 : 0;
    }
    return 1;
}

static rv_voice_result_t rv_encode_and_queue_voice(rv_voice_t* v,
                                                  const int16_t* samples,
                                                  uint32_t sample_count)
{
    if (!rv_capture_should_transmit(v)) return RV_VOICE_OK;

    uint8_t opus[RV_OPUS_MAX_PACKET];
    int olen = rv_opus_encode(v->enc, samples, (int)sample_count, opus, (int)sizeof(opus));
    if (olen <= 0) {
        rv_emit_error(v, RV_VOICE_ERR_INTERNAL, "opus encode failed");
        return RV_VOICE_ERR_INTERNAL;
    }

    uint8_t flags = 0;
    rv_get_tx_flags(v, NULL, NULL, NULL, &flags);

    uint8_t pkt[RV_MAX_PKT_SIZE];
    uint16_t seq = v->seq++;

    int pkt_len = rv_build_voice_packet_ex(pkt, (int)sizeof(pkt),
                                          v->player_id, seq,
                                          flags,
                                          opus, (uint16_t)olen);
    if (pkt_len <= 0) {
        rv_emit_error(v, RV_VOICE_ERR_INTERNAL, "build voice packet failed");
        return RV_VOICE_ERR_INTERNAL;
    }

    if (!out_push(v, pkt, (uint32_t)pkt_len)) {
        // dropping is expected under congestion; keep engine realtime
        rv_emit_log(v, 1, "outgoing queue full (dropping voice)");
        return RV_VOICE_OK;
    }

    return RV_VOICE_OK;
}

/* ============================================================
   Public API
   ============================================================ */

rv_voice_t* rv_voice_create(const rv_voice_config_t* cfg,
                            const rv_voice_allocators_t* allocs,
                            const rv_voice_callbacks_t* cbs)
{
    if (!cfg) return NULL;

    // API compatibility: accept older minor versions
    if (rv_ver_major(cfg->api_version) != RV_VOICE_API_VERSION_MAJOR) return NULL;
    if (rv_ver_minor(cfg->api_version) > RV_VOICE_API_VERSION_MINOR) return NULL;
    if (cfg->max_players == 0) return NULL;

    rv_voice_allocators_t use_allocs;
    memset(&use_allocs, 0, sizeof(use_allocs));
    if (allocs) use_allocs = *allocs;

    rv_voice_t* v = (rv_voice_t*)rv_alloc_raw(&use_allocs, sizeof(rv_voice_t));
    if (!v) return NULL;
    memset(v, 0, sizeof(*v));

    v->allocs = use_allocs;
    if (cbs) v->cbs = *cbs;

    v->cfg = *cfg;

    // Sanitize capture_mode (older callers may not set it)
    if (v->cfg.capture_mode != RV_VOICE_CAPTURE_ALWAYS_ON &&
        v->cfg.capture_mode != RV_VOICE_CAPTURE_PTT_ONLY) {
        v->cfg.capture_mode = RV_VOICE_CAPTURE_PTT_ONLY;
    }

    // mono output for now
    v->opus_cfg.sample_rate  = (int)v->cfg.sample_rate_hz;
    v->opus_cfg.channels     = 1;
    v->opus_cfg.frame_ms     = (int)v->cfg.frame_ms;
    v->opus_cfg.bitrate_bps  = 20000;
    v->opus_cfg.use_fec      = 0;

    v->frame_samples = (v->cfg.sample_rate_hz * v->cfg.frame_ms) / 1000u;
    if (v->frame_samples == 0 || v->frame_samples > RV_CAPTURE_MAX_SAMPLES) {
        rv_free_raw(&use_allocs, v);
        return NULL;
    }

    rv_eventq_init(&v->evq);
    rv_ring_init(&v->cap_q);

    // default local state: PTT up, radio off, channel 0
    memset(&v->local_state, 0, sizeof(v->local_state));
    v->has_local_state = 0;

    v->enc = rv_opus_enc_create(&v->opus_cfg);
    if (!v->enc) {
        rv_free_mem(v, v);
        return NULL;
    }

    const uint32_t n = v->cfg.max_players;

    v->dec           = (rv_opus_dec_t**)rv_alloc_mem(v, sizeof(rv_opus_dec_t*) * n);
    v->jb            = (rv_opus_jitter_t*)rv_alloc_mem(v, sizeof(rv_opus_jitter_t) * n);
    v->pcm_buf       = (int16_t**)rv_alloc_mem(v, sizeof(int16_t*) * n);
    v->pcm_count     = (uint32_t*)rv_alloc_mem(v, sizeof(uint32_t) * n);
    v->speaking      = (uint8_t*)rv_alloc_mem(v, sizeof(uint8_t) * n);
    v->last_rx_ms    = (uint32_t*)rv_alloc_mem(v, sizeof(uint32_t) * n);
    v->last_rx_flags = (uint8_t*)rv_alloc_mem(v, sizeof(uint8_t) * n);

    if (!v->dec || !v->jb || !v->pcm_buf || !v->pcm_count || !v->speaking || !v->last_rx_ms || !v->last_rx_flags) {
        rv_voice_destroy(v);
        return NULL;
    }

    memset(v->dec, 0, sizeof(rv_opus_dec_t*) * n);
    memset(v->pcm_buf, 0, sizeof(int16_t*) * n);
    memset(v->pcm_count, 0, sizeof(uint32_t) * n);
    memset(v->speaking, 0, sizeof(uint8_t) * n);
    memset(v->last_rx_ms, 0, sizeof(uint32_t) * n);
    memset(v->last_rx_flags, 0, sizeof(uint8_t) * n);

    for (uint32_t i = 0; i < n; ++i) {
        v->dec[i] = rv_opus_dec_create(&v->opus_cfg);
        rv_opus_jitter_init(&v->jb[i]);

        v->pcm_buf[i] = (int16_t*)rv_alloc_mem(v, sizeof(int16_t) * v->frame_samples);
        if (!v->dec[i] || !v->pcm_buf[i]) {
            rv_voice_destroy(v);
            return NULL;
        }
    }

    v->initialized = 1;
    rv_emit_log(v, 0, "rv_voice_create: initialized");
    return v;
}

void rv_voice_destroy(rv_voice_t* v) {
    if (!v) return;

    if (v->enc) rv_opus_enc_destroy(v->enc);

    if (v->dec) {
        for (uint32_t i = 0; i < v->cfg.max_players; ++i) {
            if (v->dec[i]) rv_opus_dec_destroy(v->dec[i]);
        }
    }

    if (v->pcm_buf) {
        for (uint32_t i = 0; i < v->cfg.max_players; ++i) {
            if (v->pcm_buf[i]) rv_free_mem(v, v->pcm_buf[i]);
        }
    }

    rv_free_mem(v, v->dec);
    rv_free_mem(v, v->jb);
    rv_free_mem(v, v->pcm_buf);
    rv_free_mem(v, v->pcm_count);
    rv_free_mem(v, v->speaking);
    rv_free_mem(v, v->last_rx_ms);
    rv_free_mem(v, v->last_rx_flags);

    rv_free_mem(v, v);
}

rv_voice_result_t rv_voice_connect(rv_voice_t* v, const rv_voice_connect_info_t* info) {
    if (!v || !info) return RV_VOICE_ERR_INVALID_ARGUMENT;
    if (!v->initialized) return RV_VOICE_ERR_NOT_INITIALIZED;

    // Provided by host; never hardcoded in engine
    v->session_id = info->session_id;
    v->player_id  = info->player_id;

    uint8_t pkt[64];
    int pkt_len = rv_build_join_packet(pkt, (int)sizeof(pkt), info->session_id, info->player_id);
    if (pkt_len <= 0) {
        rv_emit_error(v, RV_VOICE_ERR_INTERNAL, "rv_voice_connect: failed to build join packet");
        return RV_VOICE_ERR_INTERNAL;
    }

    if (!out_push(v, pkt, (uint32_t)pkt_len)) {
        rv_emit_error(v, RV_VOICE_ERR_INTERNAL, "rv_voice_connect: outgoing queue full");
        return RV_VOICE_ERR_INTERNAL;
    }

    v->connected = 1;

    rv_voice_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = RV_VOICE_EVENT_CONNECTED;
    (void)rv_eventq_push(&v->evq, &ev);

    return RV_VOICE_OK;
}

rv_voice_result_t rv_voice_disconnect(rv_voice_t* v) {
    if (!v) return RV_VOICE_ERR_INVALID_ARGUMENT;

    v->connected = 0;

    rv_voice_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = RV_VOICE_EVENT_DISCONNECTED;
    (void)rv_eventq_push(&v->evq, &ev);

    return RV_VOICE_OK;
}

rv_voice_result_t rv_voice_set_local_state(rv_voice_t* v, const rv_voice_player_state_t* st) {
    if (!v || !st) return RV_VOICE_ERR_INVALID_ARGUMENT;
    v->local_state = *st;
    v->has_local_state = 1;
    return RV_VOICE_OK;
}

/*
 * Thread-safe (SPSC) capture submit.
 * Intended for audio callback thread (single producer) while game thread calls rv_voice_tick (single consumer).
 * Drops silently if full (realtime safe).
 */
rv_voice_result_t rv_voice_submit_capture_pcm_async(rv_voice_t* v,
                                                   const int16_t* samples,
                                                   uint32_t sample_count)
{
    if (!v || !samples || sample_count == 0) return RV_VOICE_ERR_INVALID_ARGUMENT;
    if (!v->initialized) return RV_VOICE_ERR_NOT_INITIALIZED;
    if (!v->connected) return RV_VOICE_ERR_NOT_CONNECTED;

    // Keep engine timing stable: require exact frame size
    if (sample_count != v->frame_samples) return RV_VOICE_ERR_INVALID_ARGUMENT;

    (void)rv_ring_push(&v->cap_q, samples, sample_count);
    return RV_VOICE_OK;
}

/*
 * Non-async submit: route through the same queue so behavior is consistent everywhere.
 * If you want "direct encode" on the calling thread, change this back — but then you have two code paths.
 */
rv_voice_result_t rv_voice_submit_capture_pcm(rv_voice_t* v,
                                             const int16_t* samples,
                                             uint32_t sample_count)
{
    return rv_voice_submit_capture_pcm_async(v, samples, sample_count);
}

rv_voice_result_t rv_voice_ingest_packet(rv_voice_t* v,
                                        const uint8_t* data,
                                        uint32_t size,
                                        uint32_t now_ms)
{
    if (!v || !data || size == 0) return RV_VOICE_ERR_INVALID_ARGUMENT;
    if (!v->initialized) return RV_VOICE_ERR_NOT_INITIALIZED;

    rv_pkt_hdr_t hdr;
    if (rv_parse_packet_header(data, (int)size, &hdr) != 0)
        return RV_VOICE_OK;

    if (hdr.type != RV_PKT_VOICE) return RV_VOICE_OK;

    uint16_t speaker_id = 0, seq = 0;
    uint8_t flags = 0;
    const uint8_t* payload = NULL;
    uint16_t payload_len = 0;

    if (rv_parse_voice_packet(data, (int)size, &speaker_id, &seq, &flags, &payload, &payload_len) != 0)
        return RV_VOICE_OK;

    if (speaker_id == 0 || speaker_id > (uint16_t)v->cfg.max_players)
        return RV_VOICE_OK;

    uint32_t idx = (uint32_t)(speaker_id - 1u);

    v->last_rx_flags[idx] = flags;
    rv_opus_jitter_push(&v->jb[idx], seq, payload, payload_len);
    v->last_rx_ms[idx] = now_ms;

    if (!v->speaking[idx]) {
        v->speaking[idx] = 1;

        rv_voice_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = RV_VOICE_EVENT_SPEAKING;
        ev.as.speaking.speaker_id = speaker_id;
        ev.as.speaking.is_speaking = 1;
        (void)rv_eventq_push(&v->evq, &ev);
    }

    return RV_VOICE_OK;
}

int rv_voice_poll_outgoing(rv_voice_t* v,
                           uint8_t* out_buf,
                           uint32_t out_buf_cap,
                           uint32_t* out_size)
{
    return out_pop(v, out_buf, out_buf_cap, out_size);
}

rv_voice_result_t rv_voice_tick(rv_voice_t* v, uint32_t now_ms) {
    if (!v) return RV_VOICE_ERR_INVALID_ARGUMENT;
    if (!v->initialized) return RV_VOICE_ERR_NOT_INITIALIZED;

    const uint32_t n = v->cfg.max_players;
    const uint32_t speaking_timeout_ms = 250u;

    /* ------------------------------------------------------------
       1) Drain async capture queue and transmit (voice thread)
       ------------------------------------------------------------ */
    rv_pcm_frame_t f;
    while (rv_ring_pop(&v->cap_q, &f)) {
        // f.count is validated by push; still sanity check
        if (f.count != v->frame_samples) continue;
        (void)rv_encode_and_queue_voice(v, f.samples, f.count);
    }

    /* ------------------------------------------------------------
       2) Decode incoming per-speaker frames -> emit PCM events
       ------------------------------------------------------------ */
    for (uint32_t i = 0; i < n; ++i) v->pcm_count[i] = 0;

    for (uint32_t i = 0; i < n; ++i) {
        // speaking timeout -> speaking event off
        if (v->speaking[i] && (now_ms - v->last_rx_ms[i] > speaking_timeout_ms)) {
            v->speaking[i] = 0;

            rv_voice_event_t ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = RV_VOICE_EVENT_SPEAKING;
            ev.as.speaking.speaker_id = (uint16_t)(i + 1u);
            ev.as.speaking.is_speaking = 0;
            (void)rv_eventq_push(&v->evq, &ev);
        }

        const uint8_t* pkt = NULL;
        uint16_t pkt_len = 0;

        if (!rv_opus_jitter_pop(&v->jb[i], &pkt, &pkt_len))
            continue;

        // pkt==NULL && pkt_len==0 => PLC is allowed by your decoder; it should output comfort noise/silence
        int decoded = rv_opus_decode(v->dec[i],
                                     pkt, (int)pkt_len,
                                     v->pcm_buf[i],
                                     (int)v->frame_samples);
        if (decoded <= 0) continue;

        v->pcm_count[i] = (uint32_t)decoded;

        const uint8_t flags = v->last_rx_flags[i];
        const uint8_t ch = rv_flags_channel(flags);

        rv_voice_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = RV_VOICE_EVENT_PCM_FRAME;
        ev.as.pcm.speaker_id = (uint16_t)(i + 1u);
        ev.as.pcm.sample_rate = v->cfg.sample_rate_hz;
        ev.as.pcm.channels = 1;

        ev.as.pcm.flags = flags;
        ev.as.pcm.radio_channel = ch;

        ev.as.pcm.samples = v->pcm_buf[i];
        ev.as.pcm.sample_count = (uint32_t)decoded;
        (void)rv_eventq_push(&v->evq, &ev);
    }

    return RV_VOICE_OK;
}

int rv_voice_poll_event(rv_voice_t* v, rv_voice_event_t* out_event) {
    if (!v || !out_event) return 0;
    return rv_eventq_pop(&v->evq, out_event);
}

/* ============================================================
   Mixed output
   ============================================================ */

static int16_t clamp16(int x) {
    if (x > INT16_MAX) return INT16_MAX;
    if (x < INT16_MIN) return INT16_MIN;
    return (int16_t)x;
}

int rv_voice_mix_output(rv_voice_t* v,
                        int16_t* out_pcm,
                        uint32_t out_samples_per_ch)
{
    if (!v || !out_pcm) return -1;
    if (!v->initialized) return -2;
    if (v->opus_cfg.channels != 1) return -3;

    memset(out_pcm, 0, sizeof(int16_t) * out_samples_per_ch);

    const uint32_t n = v->cfg.max_players;
    uint32_t any = 0;

    for (uint32_t i = 0; i < n; ++i) {
        uint32_t cnt = v->pcm_count[i];
        if (cnt == 0) continue;

        any = 1;
        uint32_t mix_n = (cnt < out_samples_per_ch) ? cnt : out_samples_per_ch;

        for (uint32_t s = 0; s < mix_n; ++s) {
            int sum = (int)out_pcm[s] + (int)v->pcm_buf[i][s];
            out_pcm[s] = clamp16(sum);
        }
    }

    return any ? (int)out_samples_per_ch : 0;
}
