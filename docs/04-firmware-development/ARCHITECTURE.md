# JrBot Architecture

## Overview

JrBot separates physical hardware, safe capabilities, and control interfaces.

```text
Interfaces / App / Panel / future AI systems
                  ↓
         Controlled capabilities
                  ↓
        Modes / physical resources
                  ↓
               ESP32-S3
      OLED · camera · audio · mic
```

**Core rule:** an external layer must not have arbitrary access to GPIO, memory, or hardware drivers.

## Stable baseline — V1.7.04

### Local WebRTC

Live uses one WebRTC session for bidirectional audio and camera JPEG:

```text
OV5640 -> JPEG -> DataChannel/SCTP -> browser
MS3625 -> I2S RX -> G.711A/PCMA -> RTP/SRTP -> browser
browser -> RTP/SRTP -> G.711A/PCMA -> I2S TX -> MAX98357A
```

On HW04:

- BCLK GPIO21.
- WS/LRCLK GPIO47.
- RX SD GPIO41.
- TX DIN GPIO42.

RX and TX operate simultaneously with shared BCLK/WS.

### Mode Manager

V1.7.04 implements a minimal `jr_mode_manager` to track the system's logical operating mode.

Defined modes:

```text
IDLE
AUTONOMOUS
LIVE
RECORDING
PLAYBACK
DIAGNOSTIC
```

The main physically validated stable-baseline transition is `IDLE -> LIVE -> IDLE`.

### Resource Manager

The `jr_resource_manager` is the central authority for shared I2S ownership.

Defined owners:

```text
NONE
MIC
PLAYBACK
LIVE
LEGACY_VOICE
```

The manager tracks ownership and telemetry; individual drivers still configure their own physical channels.

## Runtime API

The Runtime API provides a structured contract for registered firmware functions. It allows the panel, tools, future applications, and other layers to reuse approved operations rather than gaining direct hardware access.

Current contract: [Runtime API](API_RUNTIME.md).

## V1S — JrSkill Network

V1S introduced a new architecture separating **the behavior requested by a Skill** from **how the hardware carries it out**:

```text
Declarative Skill
      ↓
Safe Skill Executor
      ↓
Capability / Runtime API
      ↓
Physical JrBot hardware
```

The Solana-based layer represents publication, versions, distribution, and licensing; it does **not** replace the local safe executor.

```text
Solana
   ↓
Skill data / license
   ↓
Local validation
   ↓
Skill Executor
   ↓
Approved capabilities
   ↓
JrBot
```

The initial JrSkill flow was validated on Devnet as part of the next defined version, **JrBot V1.8S**. Its development evidence remains in `V1s-00` and the [Colosseum archive](../06-colosseum-history/README.md).

## V2 — local voice

The V2 development line explores local voice control using the same physical capabilities. ESP-SR/MultiNet and its specific resource conflicts belong to this workstream.

## Security principles

1. External content does not write GPIO directly.
2. Available functions must be registered and allowlisted.
3. Arguments must be validated before execution.
4. Shared physical resources require explicit arbitration.
5. State and build version must be observable through logs/status.
6. Invalid configuration must not block basic robot recovery.
7. Future remote connectivity requires an authentication and security architecture of its own.

## Evolution

The architecture should evolve additively. Validated protocols and modules should not be rewritten merely to accommodate a new experimental development line.
