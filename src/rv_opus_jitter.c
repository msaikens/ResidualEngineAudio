#include "rv_opus_jitter.h"


static uint32_t slot_for_seq(uint16_t seq)
{
    return (uint32_t)(seq % RV_OPUS_JITTER_CAP);
}

static int has_future_packet(const rv_opus_jitter_t* jb, uint16_t want)
{
    if (!jb) return 0;

    /*
     * If a later packet is already buffered, allow PLC for the missing
     * sequence so playback can advance through a real packet-loss gap.
     *
     * If no future packet exists, do NOT emit PLC. Otherwise the jitter
     * buffer produces infinite silence after the stream goes idle.
     */
    for (uint32_t ahead = 1; ahead < RV_OPUS_JITTER_CAP; ++ahead)
    {
        uint16_t seq = (uint16_t)(want + ahead);
        uint32_t slot = slot_for_seq(seq);

        if (jb->packets[slot].valid && jb->packets[slot].seq == seq)
        {
            return 1;
        }
    }

    return 0;
}

void rv_opus_jitter_init(rv_opus_jitter_t* jb)
{
    if (!jb) return;

    jb->started = 0;
    jb->next_play_seq = 0;

    for (uint32_t i = 0; i < RV_OPUS_JITTER_CAP; ++i)
    {
        jb->packets[i].seq = 0;
        jb->packets[i].len = 0;
        jb->packets[i].valid = 0;

        for (uint32_t j = 0; j < RV_OPUS_MAX_PACKET; ++j)
        {
            jb->packets[i].data[j] = 0;
        }
    }
}

void rv_opus_jitter_push(rv_opus_jitter_t* jb, uint16_t seq, const uint8_t* data, uint16_t len)
{
    if (!jb || !data) return;
    if (len == 0 || len > RV_OPUS_MAX_PACKET) return;

    uint32_t slot = slot_for_seq(seq);

    jb->packets[slot].seq = seq;
    jb->packets[slot].len = len;
    jb->packets[slot].valid = 1;

    for (uint16_t i = 0; i < len; ++i)
    {
        jb->packets[slot].data[i] = data[i];
    }

    if (!jb->started)
    {
        jb->started = 1;
        jb->next_play_seq = seq;
    }
}

int rv_opus_jitter_pop(rv_opus_jitter_t* jb, const uint8_t** out_data, uint16_t* out_len)
{
    if (!jb || !out_data || !out_len) return 0;
    if (!jb->started) return 0;

    uint16_t want = jb->next_play_seq;
    uint32_t slot = slot_for_seq(want);

    if (jb->packets[slot].valid && jb->packets[slot].seq == want)
    {
        *out_data = jb->packets[slot].data;
        *out_len = jb->packets[slot].len;

        jb->packets[slot].valid = 0;
        jb->next_play_seq++;

        return 1;
    }

    if (has_future_packet(jb, want))
    {
        /*
         * Real packet loss gap: emit one PLC frame and advance.
         */
        *out_data = 0;
        *out_len = 0;

        jb->next_play_seq++;

        return 1;
    }

    /*
     * No expected packet and no future packet waiting.
     * The stream is probably idle. Do not emit endless PLC/silence.
     */
    return 0;
}