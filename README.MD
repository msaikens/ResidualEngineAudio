# Residual Voice Engine

Residual Voice Engine is a native C voice engine with Unity package bindings for **Residual**.

This repository currently provides a Windows x86_64 Unity package for microphone capture, native voice encode/decode, local packet loopback validation, and decoded PCM playback.

Current release candidate:

```text
v0.2.0-rc.1
```

## Current status

Residual Voice Engine is currently a **preview release candidate**.

Working today:

- Native Windows x64 build
- Unity package import from disk or Git URL
- Windows x86_64 Unity native plugin
- Basic Loopback sample scene
- Diagnostics sample tools
- Native/.NET smoke test
- GitHub Actions release workflow
- Stable local loopback playback after the jitter-buffer silence fix

Not included yet:

- Production Unity network transport
- Steam Networking transport
- Unity Relay transport
- Mirror/FishNet transport
- Linux/macOS native plugin builds
- Stable `1.0.0` API guarantees

## Unity package install

Install from Git using Unity Package Manager:

```text
https://github.com/msaikens/ResidualEngineAudio.git?path=/unity/com.residual.voice
```

Install this release candidate:

```text
https://github.com/msaikens/ResidualEngineAudio.git?path=/unity/com.residual.voice#v0.2.0-rc.1
```

Install from disk during local development:

```text
unity/com.residual.voice/package.json
```

## Unity samples

Samples are included under:

```text
unity/com.residual.voice/Samples~/
```

### Basic Loopback

The Basic Loopback sample validates the current voice path on one machine:

```text
Unity microphone
  → native encode
  → outgoing packet
  → local loopback transport
  → native ingest
  → native decode
  → Unity playback
```

Expected scene setup:

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

Recommended validation settings:

```text
ResidualVoiceRig
  Sample Rate Hz: 48000
  Frame Ms: 20
  Always On: enabled
  Create On Start: enabled
  Connect On Start: enabled
  Start Mic On Start: enabled
  Use Loopback Transport: enabled

ResidualVoiceMicInput
  Sample Rate Hz: 48000
  Clip Length Seconds: 2
  Auto Start: enabled
  Submit Async: enabled

ResidualVoicePlaybackSource
  Playback Sample Rate Hz: 48000
  Playback Channels: 1
  Gain: 1
  Auto Play Audio Source: enabled
  Keep Audio Source Alive: enabled
```

### Diagnostics

The Diagnostics sample contains helper components for troubleshooting Unity audio:

- `ResidualVoiceMicDeviceProbe`
- `ResidualVoicePlaybackProbe`

These are not required for normal runtime use.

Use Diagnostics if:

- Unity microphone position moves but captured peak stays at zero.
- You need to find the exact Unity microphone device name.
- You need to verify that Unity can produce audio through the current output device.

## Runtime components

Main Unity runtime components:

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

The intended runtime architecture is:

```text
Microphone PCM
  → ResidualVoiceClient
  → native voice engine
  → outgoing voice packet
  → transport send
  → transport receive
  → native packet ingest
  → decoded PCM event
  → Unity playback
```

## Networking status

The native core includes packet protocol helpers in `rv_netproto`.

That means the project already has a packet format layer, but the Unity package does **not** yet ship a production network transport.

Current package status:

```text
Transport-agnostic native voice engine with Unity bindings and local loopback validation.
```

Not yet:

```text
Drop-in multiplayer voice chat.
```

Planned transport targets may include:

- Raw UDP
- Steam Networking
- Unity Netcode for GameObjects
- Unity Relay
- Mirror
- FishNet
- EOS
- Custom relay/server transport

## Native build and smoke test

Windows requirements:

- Visual Studio C++ build tools
- CMake
- PowerShell
- .NET SDK

Run from the repository root:

```powershell
.\scripts\test-smoke-windows.ps1
```

Expected success output:

```text
Packet loopback smoke test passed.
```

If Unity is open, it may lock the native plugin DLL. Close Unity before rebuilding or copying:

```text
unity/com.residual.voice/Runtime/Plugins/Windows/x86_64/residual_voice.dll
```

## Release process

Recommended candidate versioning:

```text
0.2.0-rc.1
0.2.0-rc.2
0.2.0
```

Create and push a release tag:

```powershell
git tag -a v0.2.0-rc.1 -m "Residual Voice Unity package 0.2.0-rc.1"
git push origin v0.2.0-rc.1
```

The GitHub release workflow requires release-write permission:

```yaml
permissions:
  contents: write
```

## Platform support

Current packaged platform:

```text
Windows x86_64
```

Current Unity target:

```text
Unity 2022.3+
```

Additional platforms are planned but not yet packaged.

## Troubleshooting

### Unity imports but there is no audio

Check:

- The scene has exactly one active `AudioListener`.
- The `AudioSource` is enabled.
- The `AudioSource` is not muted.
- The `AudioSource` volume is `1`.
- The `AudioSource` spatial blend is `0`.
- Unity Game view audio is not muted.
- Windows output device is correct.

### Microphone position moves but peak stays zero

Unity is recording from a silent or wrong input device.

Use the Diagnostics sample and `ResidualVoiceMicDeviceProbe` to find the correct microphone. Copy the exact device name into `ResidualVoiceMicInput`.

### Smoke test passes but Unity still behaves like an old build

Unity may be holding the old native DLL.

Close Unity, rebuild, then reopen Unity.

## License

See `LICENSE`.

## Security

See `SECURITY.md`.