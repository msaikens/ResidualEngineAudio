# Changelog

## [0.2.0-rc.1] - 2026-07-01

### Added
- Basic Loopback sample scene for validating Unity microphone capture, native encode/decode, loopback packet flow, and playback.
- Diagnostics sample with microphone and playback probe components.
- Unity runtime components for microphone input, playback buffering, loopback transport, transport binding, push-to-talk, and rig setup.
- Windows x86_64 native plugin packaging.

### Fixed
- Stopped the native jitter buffer from emitting endless PLC silence when the stream is idle.
- Improved Unity loopback playback stability after multiple spoken phrases.

### Notes
- This is a release candidate for the Unity package preview.
- Current package target is Windows x86_64.
- Networking protocol support exists in the native core, but a production Unity network transport is not included in this release candidate.

## 0.2.0-preview.1

Added Unity runtime components:

- `IResidualVoiceTransport`
- `ResidualVoiceLoopbackTransport`
- `ResidualVoiceTransportBinding`
- `ResidualVoicePcmUtility`
- `ResidualVoicePlaybackBuffer`
- `ResidualVoicePlaybackSource`
- `ResidualVoiceMicInput`
- `ResidualVoiceRig`
- `ResidualVoicePushToTalk`

This preview makes the package closer to scene-ready inside Unity while keeping the core transport-agnostic.

Current limitations:

- Unity components are not yet validated inside a real Unity project.
- Windows x86_64 remains the only packaged native plugin target.
- No sample scene yet.
- No production network transport adapter yet.


## 0.1.0-preview.1

- Initial Unity Package Manager package layout.
- Added Windows x86_64 native plugin location.
- Added C# native wrapper.
- Added managed `ResidualVoiceClient` shell.

