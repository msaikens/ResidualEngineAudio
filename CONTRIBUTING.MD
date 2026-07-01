# Contributing to Residual Voice Engine

Thank you for your interest in contributing.

Residual Voice Engine is currently a preview-stage native voice engine and Unity package. Contributions are welcome, but the project is still moving quickly, so APIs, package structure, and transport design may change before `1.0.0`.

## Current project focus

The current focus is:

* Windows x86_64 Unity package stability
* Native voice encode/decode correctness
* Unity sample reliability
* Package import and release workflow quality
* Transport abstraction cleanup
* Two-peer local loopback validation
* Future network transport support

The package is not yet a production multiplayer voice solution.

## Before contributing

Please read:

* `README.md`
* `DOCUMENTATION.md`
* `SECURITY.md`
* `CODE_OF_CONDUCT.md`

For Unity package work, also read:

* `unity/com.residual.voice/README.md`
* `unity/com.residual.voice/CHANGELOG.md`
* `unity/com.residual.voice/Samples~/BasicLoopback/README.md`
* `unity/com.residual.voice/Samples~/Diagnostics/README.md`

## Requirements

For Windows development:

* Visual Studio C++ build tools
* CMake
* PowerShell
* .NET SDK
* Unity 2022.3 or newer

## Build and test

From the repository root:

```powershell
.\scripts\test-smoke-windows.ps1
```

Expected success output:

```text
Packet loopback smoke test passed.
```

If Unity is open, it may lock the packaged native plugin DLL. Close Unity before rebuilding or copying:

```text
unity/com.residual.voice/Runtime/Plugins/Windows/x86_64/residual_voice.dll
```

## Unity package validation

Before submitting Unity package changes:

1. Close Unity.
2. Run the smoke test.
3. Open a fresh Unity project.
4. Install the package from disk.
5. Import the Basic Loopback sample.
6. Open the Basic Loopback scene.
7. Confirm microphone capture and playback work.
8. Confirm there are no red console errors.
9. Confirm `.meta` files are included for Unity assets and sample content.

## Branch naming

Suggested branch names:

```text
feature/two-peer-loopback
feature/unity-udp-transport
fix/jitter-buffer-idle-silence
fix/unity-package-import
docs/update-package-readme
```

## Commit style

Use clear, direct commit messages.

Good examples:

```text
Fix jitter buffer idle silence
Add Unity diagnostics sample
Prepare Unity package 0.2.0-rc.1
Document Basic Loopback sample
```

Avoid vague messages like:

```text
stuff
fixes
updates
more changes
```

## Pull request checklist

Before opening a pull request, confirm:

* Native build passes.
* Smoke test passes.
* Unity package imports cleanly.
* Relevant sample scenes still work.
* Documentation is updated if behavior changed.
* `CHANGELOG.md` is updated for user-facing changes.
* Unity `.meta` files are committed when Unity assets are added or moved.
* Debug-only code is not left in runtime paths unless intentionally documented.
* Public API changes are clearly described.

## Native C guidelines

Native code should:

* Prefer explicit bounds checks.
* Treat incoming packets as untrusted.
* Avoid unsafe assumptions about packet size or payload length.
* Fail safely on malformed input.
* Keep Unity-facing APIs stable where possible.
* Avoid unnecessary platform-specific behavior in core logic.

When adding packet handling, test malformed, truncated, oversized, duplicated, delayed, and out-of-order packets.

## Unity C# guidelines

Unity runtime code should:

* Keep package runtime code separate from sample/diagnostic code.
* Avoid project-specific game assumptions in runtime components.
* Keep transport code behind `IResidualVoiceTransport`.
* Avoid blocking or heavy work in audio callbacks.
* Make inspector fields clear and safe by default.
* Auto-wire same-GameObject components where reasonable.
* Keep diagnostics in `Samples~/Diagnostics` unless they are truly runtime features.

## Documentation guidelines

Update documentation when changing:

* Public APIs
* Unity inspector setup
* Sample scene behavior
* Build scripts
* Release workflow
* Package install instructions
* Platform support
* Security-sensitive behavior
* Network transport behavior

Relevant files may include:

```text
README.md
DOCUMENTATION.md
SECURITY.md
CHANGELOG.md
unity/com.residual.voice/README.md
unity/com.residual.voice/CHANGELOG.md
unity/com.residual.voice/Samples~/BasicLoopback/README.md
unity/com.residual.voice/Samples~/Diagnostics/README.md
```

## Security reports

Do not submit security vulnerabilities through public issues or pull requests.

Follow `SECURITY.md`.

## Code of Conduct

All contributors are expected to follow `CODE_OF_CONDUCT.md`.
