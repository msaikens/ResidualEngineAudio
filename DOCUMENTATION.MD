ResidualVoiceEngine — Complete Documentation

ResidualVoiceEngine is a transport-agnostic, engine-neutral voice communication core written in C. It is designed to be embedded into any engine or runtime (Unity, Unreal, custom C/C++, C#, etc.) without committing to a specific networking stack, platform API, or audio renderer.

This document explains what the engine does, what it does not do, and how to integrate it correctly.

1. What ResidualVoiceEngine Is

ResidualVoiceEngine provides the hard, error-prone parts of voice chat:

Opus encoding and decoding

Per-speaker jitter buffers

Packet construction and parsing

Capture gating (PTT vs always-on)

Metadata flags for routing (proximity vs radio, radio channels)

Event-based decoded PCM output

Optional mixed PCM output

Lock-free async capture path for audio threads

It is not a full voice system.

2. What the Engine Does NOT Do (By Design)

ResidualVoiceEngine intentionally does not:

Open sockets

Use Steam, EOS, Unreal, or platform APIs

Perform proximity distance checks

Perform 3D spatialization or HRTF

Apply occlusion or obstruction

Encrypt packets

Authenticate users

Manage sessions, lobbies, or matchmaking

All of the above are host responsibilities.
This separation keeps the engine small, testable, portable, and future-proof.

3. High-Level Architecture

ResidualVoiceEngine follows a pull/push model.

The engine:

Produces packets

Consumes packets

Produces decoded audio events

The host:

Owns networking

Owns audio playback

Owns spatialization and routing

Typical flow:

Audio callback captures PCM

Host submits PCM to engine

Engine encodes and queues packets

Host polls outgoing packets and sends them

Host receives packets from network

Host feeds packets into engine

Engine decodes audio

Host consumes PCM events or mixed output

4. Versioning and API Stability

ResidualVoiceEngine uses major/minor API versioning.

Major mismatch → creation fails

Minor higher than supported → creation fails

Lower minor versions are accepted

Version is passed in rv_voice_config_t.api_version:

Upper 16 bits: major

Lower 16 bits: minor

This guarantees forward safety and ABI stability.

5. Core Types
Engine Handle

rv_voice_t is an opaque handle.
You never access its fields directly.

Configuration (rv_voice_config_t)

Key fields:

sample_rate_hz (recommended: 48000)

frame_ms (recommended: 20)

max_players (recommended: ≤16)

capture_mode (PTT vs always-on)

Frame size is derived as:

frame_samples = sample_rate_hz * frame_ms / 1000

This value must be respected exactly.

Capture Mode

Two capture modes are supported:

PTT_ONLY

Voice transmits only while PTT is pressed

ALWAYS_ON

Proximity chat transmits continuously

Radio chat still requires PTT

This directly supports designs like Phasmophobia.

Local Player State

The host provides player state via rv_voice_set_local_state():

ptt_down

radio_enabled

radio_channel

The engine encodes this state into packet flags.
The engine does not interpret position or orientation.

6. Threading Model (Important)

ResidualVoiceEngine is mostly single-threaded, with one exception.

Safe multi-thread usage

Audio thread:

rv_voice_submit_capture_pcm_async()

Game / network thread:

All other API calls

Async capture uses a lock-free single-producer / single-consumer ring buffer.
It never blocks and is safe for realtime audio callbacks.

What is NOT thread-safe

Calling tick, ingest, or poll concurrently

Calling engine APIs from multiple non-audio threads

If needed, the host must serialize access.

7. Lifecycle API
Create / Destroy

Allocates all internal state

Initializes Opus encoder/decoders

Initializes jitter buffers and queues

Destroy releases everything.

Connect / Disconnect

rv_voice_connect():

Stores session ID and player ID

Builds a JOIN packet

Queues it for the host to send

The engine does not open sockets or connect anywhere.

8. Audio Capture
Required frame size

Every submitted capture frame must contain exactly:

frame_samples samples per channel.

Submitting any other size is invalid.

Synchronous capture

rv_voice_submit_capture_pcm()

Applies capture gating (PTT / always-on)

Encodes immediately

Queues packets

Asynchronous capture (recommended)

rv_voice_submit_capture_pcm_async()

Safe for audio callbacks

Lock-free

Drops frames if queue is full

Encoding happens later in rv_voice_tick()

9. Networking Integration
Outgoing packets

The engine never sends packets itself.

The host polls packets:

If rv_voice_poll_outgoing() returns 1 → send packet

If 0 → no packet

If <0 → error

Incoming packets

The host feeds received packets:

rv_voice_ingest_packet(v, data, size, now_ms)

now_ms must be a monotonic millisecond clock.
It is used for speaking timeout detection.

10. Engine Tick

rv_voice_tick(v, now_ms) performs:

Encoding queued capture frames

Jitter buffer pops

Opus decode

Speaking state transitions

Event generation

Call once per frame or fixed timestep.

11. Events and Output
Event polling

Events are queued internally and polled by the host.

Event types include:

LOG

CONNECTED / DISCONNECTED

SPEAKING start/stop

PCM_FRAME

ERROR

PCM Frame Event

Each decoded frame includes:

Speaker ID

Sample rate

Channel count

Flags (radio / proximity)

Radio channel

PCM samples

PCM memory is owned by the engine and valid until the next tick.

12. Mixed Output (Optional)

The engine can mix all decoded speakers into a single mono buffer.

This is optional and intended for simple use cases.

Most games will instead spatialize per-speaker PCM.

13. Proximity vs Radio (Phasmophobia Model)

Voice packets carry routing flags:

Proximity (default)

Radio

Radio channel (0–15)

PTT state (informational)

The engine only tags packets.
The host decides:

Whether to play audio

How to attenuate it

Whether to spatialize it

Whether radio overrides distance

14. Host Responsibilities Summary

The host must provide:

Networking (UDP, Steam, EOS, etc.)

Encryption and security

Proximity distance checks

Spatialization / HRTF

Audio playback

Thread coordination

Session management

ResidualVoiceEngine intentionally does not do these things.

15. Unity Integration Notes

Use a native plugin + P/Invoke

Call async capture from audio callback

Drive tick from main thread

Route PCM into AudioSources

Apply spatialization in Unity

16. Unreal Integration Notes

Wrap engine in a module

Call tick from a component

Capture PCM via Unreal audio capture

Route decoded PCM into Audio Components

17. Security and Production Use

ResidualVoiceEngine is not secure by default.

For production, you must add:

Authentication

Encryption (DTLS / libsodium / platform crypto)

Packet validation

Replay protection

Rate limiting

18. Design Philosophy

Explicit over implicit

No hidden threads

No hidden networking

No platform lock-in

Policy belongs to the host

The engine does one thing well:
turn voice packets into decoded audio and back again.

19. Current State

At this point, the engine is:

Feature-complete for proximity + radio voice

API-stable

Thread-safe for audio capture

Engine-neutral

Ready for real integration

What remains is integration work, not engine work.