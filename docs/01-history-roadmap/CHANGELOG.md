# JrBot Changelog

This document records completed work and the available version history. It preserves the original build names for traceability. For future plans, see the [Roadmap](ROADMAP.md).

## JrSkill Network / Colosseum 2026 — Initial milestone completed (Oct 10, 2026)

The `V1s-00` branch added a **Solana Devnet-based ecosystem for declarative, licensed Skills** while preserving the existing JrBot local-control features from V1.7.04.

- **Creator Studio and JrSkill Store:** developed in the separate `JrBot_Web` repository.
- **Store backend:** `JrBot_Cloud` / Store API 0.3.0, maintained in a private repository.
- **Solana Devnet:** Skill, Offer, and License accounts; Phantom wallet, test-SOL purchases, and a demonstration of revenue sharing.
- **JrBot App:** a **My Skills** interface for discovering purchased licenses, selecting a Skill, and running authorized actions.
- **Reference builds:** firmware `JrBot_V1S_APP_03` and panel `JRBOT-PANEL-V1S-SKILLS-15`.

The [Day 8 report](../06-colosseum-history/HACKATHON_DEVLOG/day8.md) records transactions, screenshots, builds, and tests. The maintainer confirmed physical execution of a second independently authored Skill, **`jrteste`**, on **Oct 7, 2026**. The [Day 9 report](../06-colosseum-history/HACKATHON_DEVLOG/day9.md) summarizes the completed MVP and the longer-term marketplace vision.

**Scope:** the Devnet MVP was validated, but this was **not** a Solana Mainnet launch or the promotion of a stable V1.8S release. The next product version is **JrBot V1.8S**; its final integrated-release validation and promotion remain separate steps.

## JrBot_V1.7.04 — Sep 28, 2026

**Starting point:** `JrBot_V1.6.3`.

### Performance and architecture

- Removed automatic recurring `/status` polling from the App.
- Introduced the first centralized `jr_mode_manager`.
- Physically validated the `idle -> live -> idle` mode transition.
- Introduced `jr_resource_manager` for shared I2S ownership.
- Defined `mic`, `playback`, and `live` owners.
- Added telemetry for acquisition, release, busy events, and release mismatches.

### Physical validation on HW04

The tests confirmed microphone recording, audio playback, local Live WebRTC, I2S full-duplex, OV5640 camera/JPEG over DataChannel, return of I2S ownership to `none`, `i2s_release_mismatch=0`, and no reset or watchdog failure in the final test.

### Outstanding items

Acoustic echo cancellation (AEC), occasional `VIDEO_DC_DROP buffer_full` events, Internet-based WebRTC, and offline voice/ESP-SR features planned for V2.

**Official build identifier:** `JrBot_V1.7.04`. Development-branch names remain part of the historical record only.

## JrBot_V1.6.3 — Sep 23, 2026

**Starting point:** `JrBot_V1.6.2`.

### Local Live WebRTC

- Replaced the main push-to-talk flow with full-duplex WebRTC Live communication.
- Added PCMA/G.711A 8 kHz mono audio in `SEND_RECV` mode.
- Added OV5640 JPEG video over WebRTC DataChannel/SCTP.
- Added local HTTPS signaling using SSE/POST.
- Implemented fast, balanced, and quality video profiles.
- Added independent audio-direction mute and memory monitoring during a session.

### Hardware and memory

- Validated simultaneous I2S RX/TX on HW04, sharing GPIO21 (BCLK) and GPIO47 (WS).
- Preserved microphone SD on GPIO41 and MAX98357A DIN on GPIO42.
- Adjusted PSRAM allocation to preserve internal RAM during ICE/DTLS/SCTP.
- Preserved the validated camera orientation of -90°/270°.

### Physical validation

Confirmed JrBot-to-phone audio, phone-to-JrBot audio, continuous video, muting the phone microphone, and muting JrBot output audio.

### Licensing

Added the MIT License; documented contribution licensing, separate third-party licenses, and the distinction between code licensing and JrBot trademark rights.

### Known limitations

AEC was deferred; Internet WebRTC was not implemented; occasional DataChannel backpressure was observed without a noticeable effect in the recorded A/B test.

**Included historical fix:** `JrBot_V1.6.2.1` camera-rotation hotfix was absorbed into this release, rather than promoted separately.

## JrBot_V1.6.2 — Sep 19, 2026

**Starting point:** `JrBot_V1.6.1`.

- Updated `INSTALAR.bat` to discover active remote branches dynamically instead of using a fixed list.
- Made `feature/*`, `fix/*`, and `hotfix/*` branches selectable for testing while hiding `archive/*` branches.
- Added an `[ATUAL]` marker for the currently selected branch.
- Preserved the installer workflow for syncing, compiling, flashing, and opening the panel.

**Compatibility:** no hardware changes; the half-duplex push-to-talk flow, V1 audio architecture, and Runtime API contract were unchanged.

**Process:** relevant promotion work to `develop` was required to update the documentation and changelog.

## JrBot_V1.6.1 — Earlier documented version

Consolidated browser control, a default 90° camera orientation, microphone/audio, local HTTPS, and a half-duplex push-to-talk intercom. The source record does not establish an exact release date for this version.
