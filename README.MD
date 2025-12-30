# ResidualVoiceEngine (C) — transport-agnostic VoIP core

ResidualVoiceEngine is a small, embeddable **voice chat engine core** written in C. It’s designed to be **engine- and platform-neutral**: you can drive it from Unity, Unreal, a custom engine, Steam/EOS/ENet, raw UDP, etc., without committing to any single networking API.

At a high level it provides:

- **Opus encoding/decoding** (via libopus)
- **Jitter buffering** for packet reordering / loss handling
- A **packet protocol** (`rv_netproto`) with routing metadata (proximity vs “radio” channel flags)
- A **transport-agnostic interface**: the host app owns networking and simply feeds packets in / drains packets out

## What it is (and what it isn’t)

✅ **Is**: a voice codec + jitter + packetizer core, plus an optional simple UDP implementation and relay example.  
❌ **Is not**: a full “game voice system” (positional attenuation, occlusion, mixing per listener, 3D spatialization, authentication, NAT traversal, matchmaking). Those live in the host game/engine or your backend.

## Feature overview

### Voice core
- Fixed-frame audio capture (ex: 48 kHz, 20 ms)
- Opus encode into compact packets
- Per-speaker jitter buffers and decode into PCM frames
- Event stream: speaking start/stop, decoded PCM frames, logs/errors
- Optional “mix down” helper (`rv_voice_mix_output`) that sums decoded speakers into one output buffer

### Routing metadata (proximity + “radio”)
The protocol’s header includes an 8-bit `flags` field:
- `RADIO` bit and a 4-bit `RADIO_CHANNEL` (0..15)
- `PTT` bit as informational

The engine **does not** apply distance attenuation itself. Instead it emits decoded PCM frames **with flags**, so the host can route:
- **Proximity** voice → apply 3D attenuation/spatialization
- **Radio** voice → no attenuation, or special “radio” effects, and filter by channel

### Threading / realtime safety
The design supports realtime audio capture by pushing captured PCM into a **single-producer / single-consumer ring buffer** (SPSC) and encoding on the engine tick thread.

> Important: The safe threading model assumes **one capture producer thread** and **one consumer thread** calling `rv_voice_tick`. The rest of the API is intended for the “game thread”.

## Repo layout (typical)
- `include/residual_voice/voice.h` — the public engine API
- `src/voice.c` — engine implementation
- `src/rv_netproto.{c,h}` — packet protocol builder/parser (+ flags)
- `src/rv_opus.*` — Opus wrapper
- `src/rv_opus_jitter.*` — jitter buffer
- `src/rv_udp_win32.c` + `include/rv_udp.h` — optional UDP transport helper (Win32)
- `src/rv_relay*.c` — relay server example
- `examples/voice_demo.c` — example client/host application

## Building (CMake)

You’re already using CMake + Ninja + clang-cl and vcpkg for Opus. This is the minimal pattern:

```bash
cmake -S . -B build -G Ninja ^
  -DCMAKE_BUILD_TYPE=Debug ^
  -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DCMAKE_C_COMPILER="C:/Program Files/LLVM/bin/clang-cl.exe"

cmake --build build
