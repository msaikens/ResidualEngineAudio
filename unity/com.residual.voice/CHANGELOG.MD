# Changelog

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

