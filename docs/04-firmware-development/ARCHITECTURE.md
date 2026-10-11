# JrBot Firmware Architecture

## Overview

JrBot separates user interfaces, approved operations, resource coordination and physical modules.

```text
Browser / JrBot App / development panel
                  |
          Approved operations
                  |
      Runtime API and Skill executor
                  |
        Mode / resource management
                  |
               ESP32-S3
       OLED · OV5640 · I2S audio
```

External clients and Skills must not gain arbitrary access to native code, GPIO or memory.

## Main firmware components

| Component | Responsibility |
| --- | --- |
| **Camera** | OV5640 image capture and JPEG streaming |
| **Face** | OLED expressions |
| **Audio** | Microphone, playback, I2S RX/TX |
| **Portal / Live** | Local browser interface, HTTPS signaling and WebRTC |
| **Runtime API** | Structured requests restricted to registered functions |
| **Mode Manager** | Logical operating mode and transitions |
| **Resource Manager** | Ownership of shared I2S resources |

See [HW04 Wiring](../02-assembly/WIRING.md) for all GPIOs and electrical connections; they are intentionally not duplicated here.

## Live WebRTC — V1.6.3/V1.7.04 baseline

```text
OV5640 → JPEG → WebRTC DataChannel → browser
MS3625 → I2S RX → G.711A/PCMA → WebRTC audio → browser
browser → WebRTC audio → G.711A/PCMA → I2S TX → MAX98357A
```

The documented Live implementation operates on the **local network**. Audio input and output share I2S clocks on HW04. See [Project Status](../01-history-roadmap/PROJECT_STATUS.md) for current release context.

## Operating modes and resource ownership

| Manager | Defined values | Validated behavior |
| --- | --- | --- |
| `jr_mode_manager` | `IDLE`, `AUTONOMOUS`, `LIVE`, `RECORDING`, `PLAYBACK`, `DIAGNOSTIC` | `IDLE → LIVE → IDLE` on V1.7.04 |
| `jr_resource_manager` | I2S owners `NONE`, `LIVE`, `MIC`, `PLAYBACK`, `LEGACY_VOICE` | I2S ownership acquisition/release and telemetry |

Drivers configure hardware; the resource manager arbitrates access and tracks busy, acquisition, release and mismatch events. A defined mode is **not** proof that all behavior for that mode has been physically validated.

## JrSkill integration — V1.8S

```text
Store / Solana Devnet license and Skill data
                   ↓
       Browser/App verification
                   ↓
      Approved local Skill execution
                   ↓
           Runtime API / capabilities
                   ↓
                 JrBot
```

JrSkill uses **declarative, validated actions**, not uploaded native code. The Solana layer supports publishing and licensing; it does **not** replace the local executor. The first Devnet-to-robot execution flow was physically validated. See [JrSkill Network](../05-jrskill-network/README.md) for accounts, licensing, wallets and execution details.

## Technical boundaries

- Only allowlisted functions and validated arguments may reach physical modules.
- Shared audio resources must be acquired and released through the resource manager.
- An invalid configuration must not make the robot unrecoverable.
- Local command headers do not provide authentication suitable for public Internet exposure.
- V2 offline-voice development remains a **separate workstream**, not an existing V1.8S capability.

For supported requests and errors, use [Runtime API](API_RUNTIME.md). For the distinction between compilation, simulation and physical tests, use [Testing](TESTING.md).
