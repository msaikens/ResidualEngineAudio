# Security Policy

## Supported versions

Residual Voice Engine is currently pre-`1.0.0`.

Security fixes are prioritized for the latest release candidate and the active development branch.

| Version | Supported |
|---|---:|
| `0.2.0-rc.x` | Yes |
| `0.2.0-preview.x` | Limited |
| `< 0.2.0` | No |

Older preview builds may not receive backported fixes.

## Reporting a vulnerability

Please do **not** report security vulnerabilities through public GitHub Issues.

Use GitHub private vulnerability reporting if it is enabled for this repository.

If private vulnerability reporting is not available, contact the maintainer directly.

Include:

- Affected version or commit SHA
- Operating system
- Unity version, if applicable
- Steps to reproduce
- Proof-of-concept input, packet, scene, or script when safe to provide
- Expected impact
- Whether the issue is local-only or remotely triggerable
- Suggested fix, if known

## Security-sensitive areas

Important areas include:

- Native packet parsing
- Voice packet ingest
- Buffer bounds checks
- Packet payload size checks
- Opus packet handling
- Jitter buffer behavior
- Native plugin loading
- Unity package import behavior
- Build scripts
- GitHub Actions release permissions
- Future network transport implementations

## Out of scope

The following are generally not treated as security vulnerabilities unless they can be chained into a real exploit:

- General audio quality problems
- Local Unity scene misconfiguration
- Expected microphone permission prompts
- Unsupported platform failures
- Debug-only sample behavior
- Issues caused by modifying package files outside documented setup

## Packet handling expectations

Incoming packets must be treated as untrusted.

Packet ingest code should validate:

- Packet type
- Packet size
- Payload size
- Sequence number behavior
- Speaker/player identifiers
- Reserved fields
- Codec payload limits

Malformed packets should fail safely and must not crash the host process.

## Build and release security

Release workflows should use least-privilege permissions.

Workflows that create GitHub Releases require:

```yaml
permissions:
  contents: write
```

Avoid broader permissions unless specifically required.

Release artifacts should be generated from tagged commits.

Packaged native plugins should match the source used for the release tag.

## Current limitations

Current release-candidate limitations:

- Windows x86_64 only
- No production Unity network transport included
- Public API may change before `1.0.0`
- Packet behavior may change before `1.0.0`

Before exposing Residual Voice Engine to real network traffic, test packet ingest with:

- Malformed packets
- Truncated packets
- Oversized packets
- Duplicated packets
- Delayed packets
- Out-of-order packets
- Unknown packet types
- Invalid player IDs
- Invalid payload lengths

## Disclosure expectations

The maintainer will try to:

- Acknowledge valid reports
- Reproduce and triage the issue
- Avoid public disclosure until a fix or mitigation is available
- Credit reporters when appropriate and desired
- Release a patched version or document mitigations

No guaranteed response timeline is currently promised because the project is still early-stage.