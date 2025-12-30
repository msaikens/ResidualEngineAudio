#pragma once
#include "residual_voice/voice.h"
#include <stdint.h>

#ifndef RV_EVENT_QUEUE_CAP
#define RV_EVENT_QUEUE_CAP 256
#endif

typedef struct rv_event_queue {
    rv_voice_event_t buf[RV_EVENT_QUEUE_CAP];
    uint32_t r;
    uint32_t w;
} rv_event_queue_t;

static inline void rv_eventq_init(rv_event_queue_t* q) { q->r = q->w = 0; }

static inline uint32_t rv_eventq_count(const rv_event_queue_t* q) { return (q->w - q->r); }

static inline int rv_eventq_push(rv_event_queue_t* q, const rv_voice_event_t* ev) {
    if (rv_eventq_count(q) >= RV_EVENT_QUEUE_CAP) return 0;
    q->buf[q->w % RV_EVENT_QUEUE_CAP] = *ev;
    q->w++;
    return 1;
}

static inline int rv_eventq_pop(rv_event_queue_t* q, rv_voice_event_t* out) {
    if (q->r == q->w) return 0;
    *out = q->buf[q->r % RV_EVENT_QUEUE_CAP];
    q->r++;
    return 1;
}
