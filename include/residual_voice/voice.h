#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RV_VOICE_API_VERSION_MAJOR 2u
#define RV_VOICE_API_VERSION_MINOR 1u
#define RV_VOICE_API_VERSION ((RV_VOICE_API_VERSION_MAJOR << 16) | RV_VOICE_API_VERSION_MINOR)

/* ===========================
   Voice routing flags
   (mirrors wire format, public API)
   =========================== */

// bit0: RADIO (1=radio, 0=proximity)
// bits1-4: RADIO_CHANNEL (0..15)
// bit5: PTT (informational)
// bits6-7: reserved

#define RV_VOICE_FLAG_RADIO         0x01u
#define RV_VOICE_FLAG_CH_SHIFT      1u
#define RV_VOICE_FLAG_CH_MASK       (0x0Fu << RV_VOICE_FLAG_CH_SHIFT)
#define RV_VOICE_FLAG_PTT           0x20u



typedef struct rv_voice rv_voice_t;

typedef struct rv_vec3 {
    float x, y, z;
} rv_vec3_t;

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
_Static_assert(sizeof(rv_vec3_t) == 12, "rv_vec3_t size must be 12 bytes");
#endif

typedef enum rv_voice_result {
    RV_VOICE_OK = 0,
    RV_VOICE_ERR_INVALID_ARGUMENT = -1,
    RV_VOICE_ERR_OUT_OF_MEMORY = -2,
    RV_VOICE_ERR_NOT_INITIALIZED = -3,
    RV_VOICE_ERR_NOT_CONNECTED = -4,
    RV_VOICE_ERR_INTERNAL = -100
} rv_voice_result_t;

typedef enum rv_voice_event_type {
    RV_VOICE_EVENT_NONE = 0,
    RV_VOICE_EVENT_LOG,
    RV_VOICE_EVENT_CONNECTED,
    RV_VOICE_EVENT_DISCONNECTED,
    RV_VOICE_EVENT_SPEAKING,
    RV_VOICE_EVENT_PCM_FRAME,
    RV_VOICE_EVENT_ERROR
} rv_voice_event_type_t;

typedef struct rv_voice_event_log {
    int level;            // 0=info,1=warn,2=error
    const char* message;  // owned by engine until next poll
} rv_voice_event_log_t;

typedef struct rv_voice_event_speaking {
    uint16_t speaker_id;
    uint8_t is_speaking;
} rv_voice_event_speaking_t;

typedef enum rv_voice_capture_mode {
    // Back-compat / simplest: only transmit when local_state.ptt_down == 1
    RV_VOICE_CAPTURE_PTT_ONLY = 0,

    // Proximity is always allowed; radio still requires PTT
    RV_VOICE_CAPTURE_ALWAYS_ON = 1,
} rv_voice_capture_mode_t;

typedef struct rv_voice_event_pcm {
    uint16_t speaker_id;
    uint32_t sample_rate;
    uint8_t channels;

    uint8_t  reserved_u8[2]; // must be zero
    // NEW: routing metadata
    uint8_t flags;         // RV_FLAG_* from rv_netproto.h
    uint8_t radio_channel; // 0..15 (only meaningful if flags has RV_FLAG_RADIO)

    const int16_t* samples;  // owned by engine until next tick overwrite
    uint32_t sample_count;   // per-channel
} rv_voice_event_pcm_t;

typedef struct rv_voice_event_error {
    rv_voice_result_t code;
    const char* message; // owned by engine until next poll
} rv_voice_event_error_t;

typedef struct rv_voice_event {
    rv_voice_event_type_t type;
    union {
        rv_voice_event_log_t log;
        rv_voice_event_speaking_t speaking;
        rv_voice_event_pcm_t pcm;
        rv_voice_event_error_t error;
    } as;
} rv_voice_event_t;

typedef void* (*rv_alloc_fn)(void* user, size_t size);
typedef void  (*rv_free_fn)(void* user, void* ptr);
typedef void  (*rv_log_fn)(void* user, int level, const char* msg);

typedef struct rv_voice_allocators {
    void* user;
    rv_alloc_fn alloc;
    rv_free_fn free;
} rv_voice_allocators_t;

typedef struct rv_voice_callbacks {
    void* user;
    rv_log_fn log;
} rv_voice_callbacks_t;

typedef struct rv_voice_config {
    uint32_t api_version;
    uint32_t sample_rate_hz;   // 48000
    uint32_t frame_ms;         // 20
    uint32_t max_players;      // <=16
    uint32_t jitter_target_ms; // unused for now
    uint32_t jitter_max_ms;    // unused for now
    rv_voice_capture_mode_t capture_mode;
    uint32_t reserved_u32[8];
} rv_voice_config_t;

typedef struct rv_voice_connect_info {
    uint64_t session_id;

    uint16_t player_id; // 1..16

    // For now, host must be literal IP: "127.0.0.1" or "::1"
    const char* relay_host;
    uint16_t relay_port;

    // Local bind port. If 0, we bind to relay_port (handy for UDP loopback tests).
    uint16_t local_port;

    const char* token;
} rv_voice_connect_info_t;

typedef struct rv_voice_player_state {
    rv_vec3_t position;
    rv_vec3_t forward;
    uint8_t ptt_down;
    uint8_t radio_enabled;
    uint8_t radio_channel;
} rv_voice_player_state_t;

/* ===========================
   Core lifecycle
   =========================== */

rv_voice_t* rv_voice_create(const rv_voice_config_t* cfg,
                            const rv_voice_allocators_t* allocs,
                            const rv_voice_callbacks_t* cbs);

void rv_voice_destroy(rv_voice_t* v);

/* ===========================
   Session control
   =========================== */

rv_voice_result_t rv_voice_connect(rv_voice_t* v,
                                   const rv_voice_connect_info_t* info);

rv_voice_result_t rv_voice_disconnect(rv_voice_t* v);

/* ===========================
   Audio input
   =========================== */

rv_voice_result_t rv_voice_submit_capture_pcm(rv_voice_t* v,
                                              const int16_t* samples,
                                              uint32_t sample_count);
                                              /*
 * Thread-safe (SPSC) capture submit:
 * - Intended for audio callback thread (single producer).
 * - The "voice thread" (single consumer) drains it during rv_voice_tick().
 * - Never blocks; may drop if queue is full.
 *
 * sample_count must equal the engine frame size (frame_samples).
 */
rv_voice_result_t rv_voice_submit_capture_pcm_async(rv_voice_t* v,
                                                    const int16_t* samples,
                                                    uint32_t sample_count);

/* ===========================
   Player state
   =========================== */

rv_voice_result_t rv_voice_set_local_state(rv_voice_t* v,
                                           const rv_voice_player_state_t* st);

/* ===========================
   Transport-agnostic networking
   (NEW – freestanding core)
   =========================== */

/*
 * Feed a received network packet into the voice engine.
 * The host application owns all networking (UDP, Steam, EOS, etc).
 */
rv_voice_result_t rv_voice_ingest_packet(rv_voice_t* v,
                                         const uint8_t* data,
                                         uint32_t size,
                                         uint32_t now_ms);

/*
 * Poll a packet the engine wants to send.
 * Returns:
 *   1 = packet written
 *   0 = no packet available
 *  <0 = error
 */
int rv_voice_poll_outgoing(rv_voice_t* v,
                           uint8_t* out_buf,
                           uint32_t out_buf_cap,
                           uint32_t* out_size);

/* ===========================
   Engine update + events
   =========================== */

rv_voice_result_t rv_voice_tick(rv_voice_t* v, uint32_t now_ms);

int rv_voice_poll_event(rv_voice_t* v,
                        rv_voice_event_t* out_event);

// Mix decoded remote audio into a single interleaved output buffer.
// - out_pcm: destination int16 interleaved (channels = cfg.channels, currently 1)
// - out_samples_per_ch: how many samples per channel you want mixed
// - Returns samples_per_ch actually written (0 if none), or <0 on error.
//
// Notes:
// - This does NOT consume your event queue; events still work as before.
// - Mix is "best effort": missing frames use PLC silence from decoder.
int rv_voice_mix_output(rv_voice_t* v,
                        int16_t* out_pcm,
                        uint32_t out_samples_per_ch);


// Flags for voice chat PTT or ptox
static inline uint8_t rv_voice_flags_make(uint8_t is_radio,
                                          uint8_t channel,
                                          uint8_t ptt)
{
    uint8_t f = 0;
    if (is_radio) f |= RV_VOICE_FLAG_RADIO;
    f |= (uint8_t)((channel & 0x0Fu) << RV_VOICE_FLAG_CH_SHIFT);
    if (ptt) f |= RV_VOICE_FLAG_PTT;
    return f;
}

static inline uint8_t rv_voice_flags_is_radio(uint8_t f)
{
    return (uint8_t)((f & RV_VOICE_FLAG_RADIO) != 0);
}

static inline uint8_t rv_voice_flags_channel(uint8_t f)
{
    return (uint8_t)((f & RV_VOICE_FLAG_CH_MASK) >> RV_VOICE_FLAG_CH_SHIFT);
}

static inline uint8_t rv_voice_flags_ptt(uint8_t f)
{
    return (uint8_t)((f & RV_VOICE_FLAG_PTT) != 0);
}
static inline void rv_voice_config_init(rv_voice_config_t* cfg) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->api_version = RV_VOICE_API_VERSION;
    cfg->sample_rate_hz = 48000;
    cfg->frame_ms = 20;
    cfg->max_players = 16;
    cfg->capture_mode = RV_VOICE_CAPTURE_PTT_ONLY;
}

static inline void rv_voice_player_state_init(rv_voice_player_state_t* st) {
    memset(st, 0, sizeof(*st));
    st->forward.z = 1.0f; // sensible default facing forward
    st->radio_channel = 0;
}

static inline void rv_voice_connect_info_init(rv_voice_connect_info_t* ci) {
    memset(ci, 0, sizeof(*ci));
    ci->relay_host = "127.0.0.1";
    ci->relay_port = 0;   // force caller to set, unless you want a default
    ci->local_port = 0;
}

#ifdef __cplusplus
}
#endif
