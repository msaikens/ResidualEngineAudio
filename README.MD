# ResidualVoiceEngine

ResidualVoiceEngine is a small, embeddable realtime voice core written in C.

It is designed to be engine-neutral and transport-agnostic. The host game owns networking, lobbies, authentication, spatialization, and playback. ResidualVoiceEngine handles the lower-level voice pipeline:

* PCM input
* Opus encoding
* Packet creation
* Packet ingestion
* Jitter buffering
* Opus decoding
* PCM frame events
* Optional mixdown helper

The repo also includes a Unity Package Manager package under:

```text
unity/com.residual.voice
```

## Current status

This project currently supports:

* Native C voice core
* Windows x86_64 native plugin build
* Unity package skeleton
* C# P/Invoke wrapper
* Managed `ResidualVoiceClient`
* Packet loopback smoke test
* GitHub Actions Windows smoke test

Current limitations:

* Windows x86_64 only for the packaged Unity plugin
* No Unity microphone capture component yet
* No Unity playback component yet
* No Unity sample scene yet
* No built-in Steam, EOS, Mirror, FishNet, or Netcode transport adapter yet
* No NAT traversal, auth, encryption, or backend relay production system yet

## Unity Package

This repository contains an importable Unity Package Manager package for Residual Voice.

Package path:

```text
unity/com.residual.voice
```

Install from Git in Unity Package Manager:

```text
https://github.com/msaikens/ResidualEngineAudio.git?path=/unity/com.residual.voice
```

Current native plugin:

```text
unity/com.residual.voice/Runtime/Plugins/Windows/x86_64/residual_voice.dll
```

## What it is

ResidualVoiceEngine is:

* A voice codec and packetization core
* A jitter-buffered decode pipeline
* A transport-agnostic packet interface
* A native plugin foundation for Unity
* A reusable voice layer for game engines

## What it is not

ResidualVoiceEngine is not a complete game voice system by itself.

The host game or engine still needs to provide:

* Microphone capture
* Audio playback
* 3D spatialization
* Distance attenuation
* Occlusion
* Push-to-talk input
* Player/lobby/session ownership
* Network transport
* Authentication
* Encryption
* NAT traversal
* Matchmaking
* Moderation tools

## Feature overview

### Voice core

The core supports:

* Fixed-frame audio input, for example 48 kHz / 20 ms
* Opus encoding and decoding
* Packet polling through `rv_voice_poll_outgoing`
* Packet ingestion through `rv_voice_ingest_packet`
* Per-speaker jitter buffering
* Decoded PCM frame events
* Speaking events
* Log and error events
* Optional mixed output through `rv_voice_mix_output`

### Routing metadata

The packet protocol includes routing metadata for proximity and radio-style voice.

The host game can use this metadata to route decoded PCM differently:

* Proximity voice can be spatialized and attenuated by distance.
* Radio voice can bypass distance attenuation.
* Radio channels can be filtered by the host game.
* Push-to-talk state can be represented in packet metadata.

The engine does not apply 3D audio behavior itself. It emits decoded PCM and metadata so the host can decide how to route and play it.

### Threading model

The safe threading model is:

* One capture producer thread may submit PCM through the capture path.
* One consumer/game thread should call `rv_voice_tick`.
* Most API calls should happen from the game/thread owner side.
* Unity audio-thread integration should avoid heavy Unity API calls directly inside audio callbacks.

## Repo layout

```text
include/residual_voice/voice.h
src/voice.c
src/rv_netproto.c
src/rv_netproto.h
src/rv_opus.c
src/rv_opus.h
src/rv_opus_jitter.c
src/rv_opus_jitter.h
src/rv_shim_transport.c
src/rv_shim_transport.h
src/rv_shim_udp.c
src/rv_udp_win32.c
examples/
tests/ResidualVoiceSmoke/
scripts/test-smoke-windows.ps1
unity/com.residual.voice/
.github/workflows/windows-smoke.yml
```

Important files:

```text
include/residual_voice/voice.h
```

Public C API.

```text
src/voice.c
```

Main voice engine implementation.

```text
tests/ResidualVoiceSmoke
```

.NET smoke test that loads the native DLL and runs a packet loopback test.

```text
scripts/test-smoke-windows.ps1
```

Builds the native Windows plugin, copies it into the Unity package, builds the smoke test, and runs validation.

```text
unity/com.residual.voice
```

Unity Package Manager package.

## Building on Windows

Use a Visual Studio developer shell that targets x64.

From the repo root:

```powershell
.\scripts\test-smoke-windows.ps1
```

This performs the full validation flow:

1. Builds the native Release DLL.
2. Copies the DLL into the Unity package folder.
3. Builds the .NET smoke test.
4. Copies the native DLL beside the smoke test executable.
5. Runs the packet loopback smoke test.

Expected final output:

```text
Packet loopback smoke test passed.
Smoke test passed.
```

## Manual native build

The normal Windows build path is:

```powershell
cmake -S . -B build-release -A x64 -DRV_COPY_UNITY_PLUGIN=ON
cmake --build build-release --config Release
```

The generated Unity plugin should be copied to:

```text
unity/com.residual.voice/Runtime/Plugins/Windows/x86_64/residual_voice.dll
```

## Optional UDP shim

The voice core is transport-agnostic by default.

The optional Win32 UDP shim can be enabled with:

```powershell
cmake -S . -B build-udp -A x64 -DRV_BUILD_UDP_SHIM=ON
cmake --build build-udp --config Release
```

For Unity, the UDP shim should usually remain optional. Most games should route packets through their own networking layer.

## Smoke test

The smoke test validates the native/C# boundary and the voice packet path.

It checks that:

* The DLL loads from .NET.
* `rv_voice_get_api_version` works.
* Native config layout matches the C# struct layout.
* Two voice clients can be created.
* Clients can connect to the same session.
* PCM can be submitted.
* An outgoing voice packet is produced.
* The packet can be ingested by another client.
* A decoded PCM frame event is produced.

Run it through:

```powershell
.\scripts\test-smoke-windows.ps1
```

## Unity package layout

```text
unity/com.residual.voice/
  package.json
  README.md
  CHANGELOG.md
  LICENSE.md
  THIRD_PARTY_NOTICES.md
  Runtime/
    ResidualVoice.asmdef
    Plugins/
      Windows/
        x86_64/
          residual_voice.dll
    Scripts/
      ResidualVoiceNative.cs
      ResidualVoiceConfig.cs
      ResidualVoiceClient.cs
  Samples~/
    BasicLoopback/
      README.md
```

## Basic Unity usage

The current package provides the low-level client wrapper.

Example shape:

```csharp
using Residual.Voice;

var voice = new ResidualVoiceClient();

voice.Create(localPlayerId: 1);
voice.Connect(sessionId: 12345, localPlayerId: 1);

voice.PacketReady += (packet, size) =>
{
    // Send packet through your game networking layer.
};

voice.PcmFrameReady += frame =>
{
    // Route decoded PCM to Unity playback.
};

voice.Tick((uint)(UnityEngine.Time.realtimeSinceStartupAsDouble * 1000.0));
```

The future Unity integration should add:

* `ResidualVoiceMicInput`
* `ResidualVoicePlaybackSource`
* `IResidualVoiceTransport`
* UDP sample transport
* Push-to-talk component
* Proximity voice component
* Radio voice component
* Basic loopback sample scene

## Public native API highlights

Main lifecycle:

```c
rv_voice_create
rv_voice_destroy
rv_voice_connect
rv_voice_disconnect
```

Audio input:

```c
rv_voice_submit_capture_pcm
rv_voice_submit_capture_pcm_async
```

Packet flow:

```c
rv_voice_poll_outgoing
rv_voice_ingest_packet
```

Tick/events:

```c
rv_voice_tick
rv_voice_poll_event
rv_voice_poll_event_flat
```

Unity/C# helper exports:

```c
rv_voice_get_api_version
rv_voice_get_required_frame_samples
rv_voice_get_sample_rate
rv_voice_get_frame_ms
rv_voice_get_max_players
rv_voice_result_to_string
```

## GitHub Actions

The repo includes a Windows smoke-test workflow:

```text
.github/workflows/windows-smoke.yml
```

The workflow builds the native plugin, runs the packet loopback smoke test, and uploads the Windows plugin artifact.

## Versioning

Current preview package version:

```text
0.1.0-preview.1
```

Suggested tag:

```text
v0.1.0-preview.1
```

## Third-party software

ResidualVoiceEngine uses Opus for voice encoding and decoding.

See:

```text
THIRD_PARTY_NOTICES.md
```

## License

MIT. See:

```text
LICENSE
```
