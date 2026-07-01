# Residual Voice Engine Documentation

Residual Voice Engine is a native C voice engine with Unity package bindings for **Residual**.

This document describes the current project architecture, Unity package layout, native build process, validation workflow, release process, and near-term roadmap.

Current candidate:

```text
v0.2.0-rc.1
```

## 1. Project status

Residual Voice Engine is currently a **preview release candidate**.

The current package is suitable for:

- Fresh Unity package import testing
- Windows x86_64 local loopback validation
- Microphone capture validation
- Native encode/decode validation
- Unity decoded PCM playback validation
- Package release workflow testing

The current package is **not yet** a drop-in production multiplayer voice solution.

Not included yet:

- Production Unity network transport
- Steam Networking transport
- Unity Relay transport
- Mirror/FishNet transport
- Linux/macOS native plugin builds
- Stable `1.0.0` API guarantees

## 2. Repository layout

```text
.
├── include/
│   └── residual_voice/
│       └── voice.h
├── src/
│   ├── voice.c
│   ├── rv_netproto.c
│   ├── rv_netproto.h
│   ├── rv_opus_jitter.c
│   └── rv_opus_jitter.h
├── tests/
│   └── ResidualVoiceSmoke/
├── scripts/
│   └── test-smoke-windows.ps1
├── unity/
│   └── com.residual.voice/
│       ├── package.json
│       ├── README.md
│       ├── CHANGELOG.md
│       ├── LICENSE.md
│       ├── Runtime/
│       │   ├── ResidualVoice.asmdef
│       │   ├── Plugins/
│       │   │   └── Windows/x86_64/residual_voice.dll
│       │   └── Scripts/
│       └── Samples~/
│           ├── BasicLoopback/
│           └── Diagnostics/
├── .github/
│   └── workflows/
├── README.md
├── DOCUMENTATION.md
└── SECURITY.md
```

## 3. Native engine overview

The native engine owns the low-level voice pipeline.

High-level native responsibilities:

- Voice client creation and destruction
- PCM frame submission
- Voice packet generation
- Voice packet ingest
- Opus encode/decode path
- Jitter buffering
- Flat event polling for Unity/C# interop
- Protocol metadata for future networking

The native public API is exposed through:

```text
include/residual_voice/voice.h
```

Important native concepts:

```text
rv_voice_t
rv_voice_config_t
rv_voice_connect_info_t
rv_voice_player_state_t
rv_voice_event_flat_t
```

Important native calls:

```text
rv_voice_create
rv_voice_destroy
rv_voice_connect
rv_voice_submit_capture_pcm
rv_voice_tick
rv_voice_poll_outgoing_packet
rv_voice_ingest_packet
rv_voice_poll_event_flat
rv_voice_get_required_frame_samples
rv_voice_get_sample_rate
rv_voice_get_frame_ms
rv_voice_result_to_string
```

## 4. Unity package overview

The Unity package lives at:

```text
unity/com.residual.voice
```

Package name:

```text
com.residual.voice
```

Current package target:

```text
Windows x86_64
Unity 2022.3+
```

Install from Git:

```text
https://github.com/msaikens/ResidualEngineAudio.git?path=/unity/com.residual.voice
```

Install a specific release candidate:

```text
https://github.com/msaikens/ResidualEngineAudio.git?path=/unity/com.residual.voice#v0.2.0-rc.1
```

Install from disk during local development:

```text
unity/com.residual.voice/package.json
```

## 5. Unity runtime components

Main Unity runtime scripts:

```text
ResidualVoiceClient
ResidualVoiceRig
ResidualVoiceMicInput
ResidualVoicePlaybackSource
ResidualVoiceLoopbackTransport
ResidualVoiceTransportBinding
ResidualVoicePushToTalk
ResidualVoicePcmUtility
ResidualVoicePlaybackBuffer
IResidualVoiceTransport
```

### ResidualVoiceClient

Managed wrapper around the native voice engine.

Responsibilities:

- Create/destroy native client
- Submit captured PCM
- Poll outgoing packets
- Ingest incoming packets
- Poll native events
- Raise decoded PCM events
- Apply local player state

### ResidualVoiceRig

Convenience MonoBehaviour for wiring the client, microphone, playback, and transport.

Typical sample settings:

```text
Sample Rate Hz: 48000
Frame Ms: 20
Max Players: 64
Jitter Target Ms: 60
Jitter Max Ms: 200
Always On: enabled
Create On Start: enabled
Connect On Start: enabled
Start Mic On Start: enabled
Use Loopback Transport: enabled
```

### ResidualVoiceMicInput

Unity microphone capture component.

Responsibilities:

- Start Unity microphone capture
- Read microphone ring buffer
- Convert Unity float samples to PCM16
- Accumulate exact native frame sizes
- Submit frames to `ResidualVoiceClient`

Expected frame size for 48 kHz / 20 ms:

```text
960 samples
```

### ResidualVoicePlaybackSource

Unity decoded PCM playback component.

Responsibilities:

- Receive decoded PCM frames
- Queue per-speaker playback buffers
- Mix decoded PCM into Unity audio output
- Keep playback source alive

Requires:

```text
AudioSource
```

### ResidualVoiceLoopbackTransport

Local testing transport.

It sends outgoing packets back into the same local client. This validates the package pipeline but does not represent real multiplayer behavior.

### ResidualVoiceTransportBinding

Binds a voice client to an `IResidualVoiceTransport`.

High-level behavior:

```text
client outgoing packet → transport send
transport receive → client ingest packet
client tick → decoded PCM event
```

### ResidualVoicePushToTalk

Optional local state helper for proximity/radio push-to-talk testing.

## 6. Unity sample scenes

Samples live under:

```text
unity/com.residual.voice/Samples~/
```

### BasicLoopback

Purpose:

Validate the package end-to-end on one machine.

Flow:

```text
Unity microphone
  → native encode
  → outgoing packet
  → local loopback transport
  → native ingest
  → native decode
  → Unity playback
```

Expected scene:

```text
Main Camera
  - Camera
  - Audio Listener

Residual Voice Basic Loopback
  - AudioSource
  - ResidualVoiceRig
  - ResidualVoiceMicInput
  - ResidualVoicePlaybackSource
```

Validation signs:

```text
Microphone Position Debug moves
Peak Level Debug rises when speaking
Submitted Frame Count increases
Queued Frame Count increases when decoded PCM arrives
Last Queued Peak Debug rises when speaking
Audio playback is audible
```

### Diagnostics

Purpose:

Troubleshoot Unity audio input/output problems.

Diagnostic components:

```text
ResidualVoiceMicDeviceProbe
ResidualVoicePlaybackProbe
```

Use Diagnostics when:

- Unity microphone position moves but peak stays zero.
- You need to identify the exact Unity microphone device name.
- You need to verify that Unity audio output works.
- You need to isolate whether a problem is Unity audio setup or Residual voice pipeline behavior.

## 7. Native packet protocol

The native core includes packet protocol helpers:

```text
rv_netproto.c
rv_netproto.h
```

These files provide the packet format layer.

They do **not** provide a full network transport.

Current distinction:

```text
rv_netproto = packet format / serialization
transport = how packets move across the network
Unity integration = how game objects route voice between players
```

The current Unity package includes local loopback validation only.

Future transports may include:

- Raw UDP
- Steam Networking
- Unity Netcode for GameObjects
- Unity Relay
- Mirror
- FishNet
- EOS
- Custom relay/server transport

## 8. Intended multiplayer architecture

The target multiplayer architecture is:

```text
Client A microphone
  → Client A native encode
  → Client A outgoing packet
  → network transport
  → Client B packet receive
  → Client B native ingest
  → Client B native decode
  → Client B Unity playback
```

The current Basic Loopback sample is simpler:

```text
One local client
  → outgoing packet
  → same-client local loopback
  → ingest
  → decode
  → playback
```

The next correct networking milestone is a **two-peer local loopback** sample:

```text
Voice Sender
  → emits packets

Local Peer Bridge
  → passes packets

Voice Receiver
  → ingests and plays packets
```

This should come before UDP or Steam Networking.

## 9. Native build

Windows requirements:

- Visual Studio C++ build tools
- CMake
- PowerShell
- .NET SDK

Run from repository root:

```powershell
.\scripts\test-smoke-windows.ps1
```

Expected success:

```text
Packet loopback smoke test passed.
```

If Unity is open, it may lock:

```text
unity/com.residual.voice/Runtime/Plugins/Windows/x86_64/residual_voice.dll
```

Close Unity before rebuilding or copying the native plugin.

## 10. Smoke test

The smoke test validates native behavior outside Unity.

It should confirm:

- Native DLL builds
- Native DLL can be loaded by .NET
- Voice client can be created
- Voice client can be destroyed
- Packet loopback path works
- PCM decode event is produced

Run:

```powershell
.\scripts\test-smoke-windows.ps1
```

Passing output should include:

```text
Packet loopback smoke test passed.
```

## 11. Unity validation workflow

Use this workflow before tagging a release candidate:

1. Close Unity.
2. Run the smoke test.
3. Open a fresh Unity project.
4. Install the package from disk or Git URL.
5. Import the Basic Loopback sample.
6. Open the Basic Loopback scene.
7. Press Play.
8. Confirm microphone capture and playback.
9. Import Diagnostics only if needed.
10. Confirm no red console errors.

Fresh Git URL test:

```text
https://github.com/msaikens/ResidualEngineAudio.git?path=/unity/com.residual.voice#v0.2.0-rc.1
```

## 12. Troubleshooting

### Unity imports but there is no audio

Check:

- There is exactly one active `AudioListener`.
- `AudioSource` is enabled.
- `AudioSource` is not muted.
- `AudioSource` volume is `1`.
- `AudioSource` spatial blend is `0`.
- Unity Game view audio is not muted.
- Windows output device is correct.
- The Basic Loopback sample works before testing a custom scene.

### Microphone position moves but peak stays zero

Unity is recording from a silent or wrong input device.

Use:

```text
Samples~/Diagnostics/ResidualVoiceMicDeviceProbe
```

Find the device where peak rises while speaking. Copy that exact device name into:

```text
ResidualVoiceMicInput → Device Name
```

### Playback works briefly, then queues silence

This was addressed by the native jitter-buffer fix.

The jitter buffer should not emit endless PLC silence after the stream becomes idle.

If the issue returns:

- Confirm Unity is using the rebuilt DLL.
- Close Unity.
- Rebuild the native plugin.
- Reopen Unity.
- Test Basic Loopback again.

### Smoke test passes but Unity still behaves like an old build

Unity may be holding the old native DLL.

Fix:

```text
Close Unity
Run smoke/build script
Reopen Unity
Retest
```

### Release workflow fails with 403

The workflow probably lacks release write permission.

Add:

```yaml
permissions:
  contents: write
```

Also check:

```text
GitHub repo → Settings → Actions → General → Workflow permissions
```

Set workflow permissions to read/write if needed.

## 13. Release process

Recommended candidate flow:

```powershell
git status
git add .
git commit -m "Prepare Unity package 0.2.0-rc.1"
git tag -a v0.2.0-rc.1 -m "Residual Voice Unity package 0.2.0-rc.1"
git push
git push origin v0.2.0-rc.1
```

The release workflow should:

- Checkout source
- Build native plugin
- Run smoke test
- Package Unity package
- Upload artifact
- Create GitHub Release

Release workflow permission requirement:

```yaml
permissions:
  contents: write
```

## 14. Versioning

Before `1.0.0`, APIs may change.

Recommended version sequence:

```text
0.2.0-rc.1
0.2.0-rc.2
0.2.0
```

Use `rc` when the package is believed to be release-ready but still needs final validation.

Use `preview` when the package is useful but knowingly incomplete.

Examples:

```text
0.1.0-preview.1
0.2.0-preview.1
0.2.0-rc.1
0.2.0
```

## 15. Current limitations

Current limitations:

- Windows x86_64 only
- No production network transport
- No real multiplayer sample yet
- No Linux/macOS packaged native plugins
- No mobile support
- No console support
- Public API may change before `1.0.0`

## 16. Near-term roadmap

Recommended next milestones:

1. Two-peer local loopback sample
2. Local UDP transport
3. Two-editor LAN validation
4. Transport abstraction cleanup
5. Runtime packet validation hardening
6. Platform build expansion
7. Steam/Relay transport exploration
8. API stabilization for `0.3.x`
9. Wider package documentation pass
10. Stable `1.0.0` planning

## 17. Security notes

All incoming packets must be treated as untrusted.

Before enabling real network traffic, packet ingest should be tested with:

- Malformed packets
- Truncated packets
- Oversized packets
- Duplicated packets
- Delayed packets
- Out-of-order packets
- Unknown packet types
- Invalid player IDs
- Invalid payload lengths

See:

```text
SECURITY.md
```

## 18. Maintainer checklist before release

Before tagging a release candidate:

- Smoke test passes.
- Fresh Unity import passes.
- Basic Loopback sample works.
- Diagnostics sample imports.
- Package version is updated.
- CHANGELOG is updated.
- README is updated.
- DOCUMENTATION is updated.
- SECURITY is present.
- Native DLL is current.
- Unity `.meta` files are committed.
- GitHub release workflow has `contents: write`.
- Release tag is annotated.