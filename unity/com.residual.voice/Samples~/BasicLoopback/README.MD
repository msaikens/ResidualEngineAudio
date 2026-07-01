# Basic Loopback Sample

This sample is a minimal Unity scene starter for Residual Voice.

## Purpose

It demonstrates the intended local pipeline:

1. Create a `ResidualVoiceClient`.
2. Connect to a local session.
3. Capture microphone audio.
4. Convert Unity float PCM to PCM16.
5. Submit PCM to the native voice core.
6. Poll outgoing packets.
7. Loop packets back through the loopback transport.
8. Receive decoded PCM frames.
9. Play decoded PCM through Unity audio.

## Usage

After importing the sample into a Unity project:

1. Create an empty GameObject.
2. Add `ResidualVoiceBasicLoopbackDemo`.
3. Press Play.
4. Allow microphone permission if prompted.
5. Use the on-screen buttons to start, stop, or restart capture.

## Notes

This is a local loopback demo. It does not use a network transport.

For real multiplayer, replace `ResidualVoiceLoopbackTransport` with a transport adapter for your networking layer.