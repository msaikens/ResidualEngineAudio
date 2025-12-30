#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RV_VOICE_API_VERSION 1

typedef struct rv_voice rv_voice_t;

typedef struct rv_vec3 {
    float x, y, z;
} rv_vec3_t;

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

typedef struct rv_voice_event_pcm {
    uint16_t speaker_id;
    uint32_t sample_rate;
    uint8_t channels;
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

rv_voice_t* rv_voice_create(const rv_voice_config_t* cfg,
                            const rv_voice_allocators_t* allocs,
                            const rv_voice_callbacks_t* cbs);

void rv_voice_destroy(rv_voice_t* v);

rv_voice_result_t rv_voice_connect(rv_voice_t* v, const rv_voice_connect_info_t* info);
rv_voice_result_t rv_voice_disconnect(rv_voice_t* v);

rv_voice_result_t rv_voice_submit_capture_pcm(rv_voice_t* v,
                                              const int16_t* samples,
                                              uint32_t sample_count);

rv_voice_result_t rv_voice_set_local_state(rv_voice_t* v, const rv_voice_player_state_t* st);

rv_voice_result_t rv_voice_tick(rv_voice_t* v, uint32_t now_ms);

int rv_voice_poll_event(rv_voice_t* v, rv_voice_event_t* out_event);

#ifdef __cplusplus
}
#endif
