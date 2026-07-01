# Support

This document explains where to get help with Residual Voice Engine and what information to include when reporting problems.

## Current support level

Residual Voice Engine is currently pre-`1.0.0`.

Current supported target:

```text
Windows x86_64
Unity 2022.3+
```

Current supported package status:

```text
Preview release candidate
```

The current Unity package supports local loopback validation. It does not yet include a production multiplayer network transport.

## Before asking for help

Please check:

* `README.md`
* `DOCUMENTATION.md`
* `SECURITY.md`
* `unity/com.residual.voice/README.md`
* `unity/com.residual.voice/Samples~/BasicLoopback/README.md`
* `unity/com.residual.voice/Samples~/Diagnostics/README.md`

Also run the smoke test:

```powershell
.\scripts\test-smoke-windows.ps1
```

Expected success output:

```text
Packet loopback smoke test passed.
```

## Reporting bugs

Use GitHub Issues for normal bugs.

Include:

* Residual Voice Engine version or commit SHA
* Unity version
* Operating system
* Whether the issue occurs in a fresh Unity project
* Whether the Basic Loopback sample works
* Whether the Diagnostics sample works
* Full console error text
* Steps to reproduce
* Expected behavior
* Actual behavior

For native build issues, also include:

* Visual Studio version
* CMake version
* .NET SDK version
* Full build output

## Unity audio troubleshooting

If Unity imports but there is no audio, check:

* The scene has exactly one active `AudioListener`.
* The `AudioSource` is enabled.
* The `AudioSource` is not muted.
* The `AudioSource` volume is `1`.
* The `AudioSource` spatial blend is `0`.
* Unity Game view audio is not muted.
* Windows output device is correct.

If microphone position moves but peak stays zero:

1. Import the Diagnostics sample.
2. Add or open `ResidualVoiceMicDeviceProbe`.
3. Test each microphone device.
4. Find the device where peak rises while speaking.
5. Copy the exact device name into `ResidualVoiceMicInput`.

## Package install problems

For Git URL install, use:

```text
https://github.com/msaikens/ResidualEngineAudio.git?path=/unity/com.residual.voice
```

For a specific release candidate:

```text
https://github.com/msaikens/ResidualEngineAudio.git?path=/unity/com.residual.voice#v0.2.0-rc.1
```

For local development install from disk:

```text
unity/com.residual.voice/package.json
```

If Unity behaves like it is using an old native build:

1. Close Unity.
2. Rebuild or rerun the smoke script.
3. Confirm the native DLL was copied.
4. Reopen Unity.
5. Retest the Basic Loopback sample.

Unity may lock this file while open:

```text
unity/com.residual.voice/Runtime/Plugins/Windows/x86_64/residual_voice.dll
```

## Feature requests

Feature requests are welcome, but the current priority is package stability and transport architecture.

Useful feature requests include:

* Specific Unity transport integrations
* Additional platform support
* Better diagnostics
* More sample scenes
* Packet validation hardening
* Build/release workflow improvements

When requesting a transport, include:

* Target networking stack
* Unity version
* Expected player count
* Dedicated server or peer-to-peer model
* Relay/NAT requirements
* Whether proximity voice, radio voice, or both are needed

## Security issues

Do not report security vulnerabilities through public issues.

Follow `SECURITY.md`.

Security-sensitive areas include:

* Packet parsing
* Packet ingest
* Bounds checking
* Native plugin loading
* Build and release workflows
* Future network transports

## Unsupported requests

The following are not currently supported:

* Production multiplayer voice deployment help
* Console platform support
* Mobile platform support
* Non-Windows native plugin builds
* Guaranteed API stability before `1.0.0`
* Emergency support
* Private custom integration work through public issues

## Maintainer expectations

This is an early-stage project. Support is best-effort.

High-quality reports with clear reproduction steps are much more likely to be fixed quickly.
