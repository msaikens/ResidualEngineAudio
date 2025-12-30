#include "residual_voice/voice.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static void* demo_alloc(void* user, size_t sz) { (void)user; return malloc(sz); }
static void  demo_free(void* user, void* p) { (void)user; free(p); }

static void demo_log(void* user, int level, const char* msg) {
    (void)user;
    const char* tag = (level==0) ? "INFO" : (level==1) ? "WARN" : "ERR";
    printf("[%s] %s\n", tag, msg);
}

static uint32_t now_ms(void) { return (uint32_t)GetTickCount64(); }
static void sleep_ms(uint32_t ms) { Sleep(ms); }

static uint16_t parse_u16(const char* s, uint16_t def) {
    if (!s || !*s) return def;
    long v = strtol(s, NULL, 10);
    if (v < 0 || v > 65535) return def;
    return (uint16_t)v;
}
static uint64_t parse_u64(const char* s, uint64_t def) {
    if (!s || !*s) return def;
    unsigned long long v = strtoull(s, NULL, 10);
    return (uint64_t)v;
}

int main(int argc, char** argv) {
    if (argc < 5) {
        printf("usage: %s <host> <relay_port> <session_id> <player_id> [local_port]\n", argv[0]);
        printf("example: %s ::1 40000 1234 1\n", argv[0]);
        printf("example: %s ::1 40000 1234 2\n", argv[0]);
        return 1;
    }

    const char* host = argv[1];
    uint16_t relay_port = parse_u16(argv[2], 40000);
    uint64_t session_id = parse_u64(argv[3], 1234);
    uint16_t player_id  = parse_u16(argv[4], 1);
    uint16_t local_port = (argc >= 6) ? parse_u16(argv[5], 0) : 0; // default ephemeral

    rv_voice_allocators_t allocs = {0};
    allocs.alloc = demo_alloc;
    allocs.free = demo_free;

    rv_voice_callbacks_t cbs = {0};
    cbs.log = demo_log;

    rv_voice_config_t cfg = {0};
    cfg.api_version = RV_VOICE_API_VERSION;
    cfg.sample_rate_hz = 48000;
    cfg.frame_ms = 20;
    cfg.max_players = 16;

    rv_voice_t* v = rv_voice_create(&cfg, &allocs, &cbs);
    if (!v) {
        printf("Failed to create voice engine\n");
        return 2;
    }

    rv_voice_connect_info_t ci = {0};
    ci.session_id = session_id;
    ci.player_id  = player_id;     // <-- make sure your engine uses this in JOIN
    ci.relay_host = host;
    ci.relay_port = relay_port;
    ci.local_port = local_port;    // 0 = ephemeral (recommended for multi-terminal)
    ci.token = "dev-token";

    if (rv_voice_connect(v, &ci) != RV_VOICE_OK) {
        printf("connect failed\n");
        rv_voice_destroy(v);
        return 3;
    }

    // Different tone per player so you can “see” mixing is happening
    const double freq = (player_id == 1) ? 440.0 : 660.0;
    double phase = 0.0;
    const double sr = 48000.0;
    const double phase_inc = 2.0 * 3.141592653589793 * freq / sr;

    int16_t frame[960];

    uint32_t start = now_ms();
    uint32_t last_cap = start;
    uint32_t frames_out = 0;

    while (now_ms() - start < 3000) {
        uint32_t t = now_ms();

        if ((uint32_t)(t - last_cap) >= 20) {
            last_cap += 20;
            for (int i = 0; i < 960; i++) {
                double s = sin(phase);
                phase += phase_inc;
                if (phase > 2.0 * 3.141592653589793) phase -= 2.0 * 3.141592653589793;
                frame[i] = (int16_t)(s * 12000.0);
            }
            rv_voice_submit_capture_pcm(v, frame, 960);
        }

        rv_voice_tick(v, t);

        rv_voice_event_t ev;
        while (rv_voice_poll_event(v, &ev)) {
            if (ev.type == RV_VOICE_EVENT_PCM_FRAME) {
                frames_out++;
                if (frames_out % 10 == 0) {
                    printf("PCM_FRAME speaker=%u samples=%u\n",
                        (unsigned)ev.as.pcm.speaker_id,
                        (unsigned)ev.as.pcm.sample_count);
                }
            }
        }
        sleep_ms(1);
    }

    rv_voice_disconnect(v);
    rv_voice_destroy(v);
    printf("Total PCM frames out: %u\n", (unsigned)frames_out);
    return 0;
}
