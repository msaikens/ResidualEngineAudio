# Residual Voice

Residual Voice is a native Opus-based realtime voice core packaged for Unity.

## Current status

This package is currently Windows x86_64 only.

Included:

- Native `residual_voice.dll`
- C# P/Invoke wrapper
- Managed `ResidualVoiceClient`
- Packet polling API
- PCM frame event API

Not included yet:

- Unity microphone capture component
- Unity playback component
- UDP / Steam / EOS transport adapter
- Proximity spatialization component
- Radio UI / push-to-talk component

## Installation from Git

Use Unity Package Manager:

```text
https://github.com/msaikens/ResidualEngineAudio.git?path=/unity/com.residual.voice

using Residual.Voice;

var voice = new ResidualVoiceClient();

voice.Create(localPlayerId: 1);
voice.Connect(sessionId: 1234, localPlayerId: 1);

voice.PacketReady += (packet, size) =>
{
    // Send packet through your game network transport.
};

voice.PcmFrameReady += frame =>
{
    // Send decoded PCM to Unity playback.
};

voice.Tick((uint)(Time.realtimeSinceStartupAsDouble * 1000.0));

## Native platform support

## Currently packaged:

Windows x86_64

## Planned:

Linux x86_64
macOS arm64/x86_64
Android arm64-v8a
iOS

