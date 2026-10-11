# JrBot Project Status

**As of October 11, 2026.** This file describes **what JrBot currently does, the available implementation, and the remaining limitations**. See the [Roadmap](ROADMAP.md) for future goals and the [Changelog](CHANGELOG.md) for completed milestones.

## What JrBot can do

The ESP32-S3-based JrBot provides these implemented capabilities:

| Capability | Current functionality |
| --- | --- |
| **See** | Capture OV5640 photos and provide browser preview and live JPEG video |
| **Hear** | Record audio through an MS3625 I2S microphone and receive audio during local WebRTC sessions |
| **Speak** | Play sound through a MAX98357A amplifier and speaker, including browser-to-robot WebRTC audio |
| **Express** | Show selectable expressions on an SSD1306 OLED face |
| **Connect** | Wi-Fi, a browser-based control interface, local HTTPS, and WebRTC |
| **Interact** | Accept local browser commands, control Live sessions, and manage audio directions independently |
| **Run Skills** | Discover licensed declarative Skills through the JrSkill App and execute approved robot actions, validated on Solana Devnet |

The first JrSkill version permits controlled actions such as OLED `face` expressions and `wait` delays; it does not give Skills arbitrary access to native code, GPIO, or hardware drivers.

## Release and validation status

| Item | Current status |
| --- | --- |
| **Official `main` release** | `JrBot_V1.7.04` — stable local-control reference, physically validated |
| **Next product version** | **JrBot V1.8S**, incorporating JrSkill Network |
| **Integration branch** | `V1s-00`; not yet promoted to `main` |
| **JrSkill firmware build** | `JrBot_V1S_APP_03` |
| **JrSkill validation** | Store-to-robot execution, including the independently created `jrteste` Skill, confirmed on real hardware on Oct 7, 2026 |
| **Blockchain environment** | **Solana Devnet** — not a production Mainnet launch |
| **Creator platform** | Initial creation and publishing flow validated; expanded developer tools remain under development |

The V1.7.04 local-control validation and the JrSkill Devnet tests are documented milestones. **A new consolidated V1.8S release test must not be assumed until it is performed and recorded.**

## Reference hardware

**JRBOT-HW-04:** ESP32-S3 N16R8, OV5640 camera, SSD1306 OLED display, MS3625 microphone, and MAX98357A audio amplifier.

The audio interface shares BCLK (GPIO21) and WS (GPIO47); microphone data uses GPIO41 and amplifier data uses GPIO42. See the [HW04 hardware review](../02-assembly/REVISAO_HW04.md) and [wiring guide](../02-assembly/ESQUEMA_LIGACAO.md).

## Current limitations

- WebRTC Live communication currently operates **on the local network**; remote Internet access is not part of the validated release.
- Acoustic echo cancellation and occasional video DataChannel backpressure remain improvement areas.
- Local voice-command development belongs to the **V2** workstream.
- The JrSkill marketplace is validated on **Devnet**, not on Solana Mainnet.

## Where to find more

- [Roadmap](ROADMAP.md) — upcoming work
- [Changelog](CHANGELOG.md) — completed changes
- [Firmware architecture](../04-firmware-development/ARCHITECTURE.md) and [Testing](../04-firmware-development/TESTING.md) — implementation and validation
- [Hardware assembly](../02-assembly/README.md)
- [JrSkill Network](../05-jrskill-network/README.md)
- [Colosseum 2026 development evidence](../06-colosseum-history/HACKATHON_DEVLOG.md)
