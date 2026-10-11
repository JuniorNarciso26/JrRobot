# JrBot Changelog — The Journey from Prototype to Skills Ecosystem

JrBot did not start as a completed robot. It grew from **a simple expressive OLED face**, through local control, camera, audio and real-time communication, into a physical robot capable of executing licensed Skills.

This is the **chronological record of what was built and tested**. Early experiments, firmware candidates and officially promoted versions are distinguished below. The dates and milestones are based on repository commits and recorded validation; they are not invented product-release dates.

For upcoming work, see the [Roadmap](ROADMAP.md). For what is available now, see [Project Status](PROJECT_STATUS.md).

## August 2026 — The project takes shape

### August 1 — First repository record

The first recorded commit established the `JrRobot` repository. At this point, GitHub history establishes the **start of the recorded project**, not a completed firmware or a physical release.

[Initial project commit](https://github.com/JuniorNarciso26/JrRobot/commit/fa82fe81eca555d70f2494c5dd5e30659079be92)

### August 31 — JrBot gets its first face

The first documented functional firmware brought expressive eyes to an OLED connected to an **ESP32-S3**.

- Implemented an I2C OLED face with expressions controlled through Serial/USB.
- Added I2C scanning, terminal commands, and Windows/Linux scripts to build and flash the board.
- Confirmed a **physical OLED test**, with the display responding at I2C address `0x3C`.
- Established the initial division between the embedded firmware and a computer-based control interface.

This first hardware experiment used **GPIO8/GPIO9 for the OLED**. Those were *historical prototype connections*, not the final HW04 pin map.

[First physically tested face](https://github.com/JuniorNarciso26/JrRobot/commit/4da38de4251303ea840a5fa4b92a6b75d30564cf)

## September 2026 — From a face to an interactive robot

### September 1 — A visual control panel and Wi-Fi

JrBot advanced beyond typing commands into a Serial console.

- Expanded the OLED expression firmware and introduced a **local HTML control panel**.
- Added a Wi-Fi panel served by the ESP32, opening the path to browser-based interaction.
- Began packaging the firmware and simplifying installation.

An early commit called its OLED revision "face OLED v2". That was a **prototype revision**, not the later *JrBot V2 product line* for voice control.

[OLED panel milestone](https://github.com/JuniorNarciso26/JrRobot/commit/0c7fd0d4e84f1a4cf47dce4b28da5f78cf9aa1f6) · [Wi-Fi panel milestone](https://github.com/JuniorNarciso26/JrRobot/commit/170ed12dce6f2aa3592d1e6493af61d2cc614609)

### September 3–6 — Camera, sound and reference hardware

The robot began gaining the hardware interfaces needed to see and communicate.

- Added panel-driven camera capture and tests for the **MAX98357A** audio amplifier.
- Refactored the firmware toward modular components.
- Improved installation, diagnostics, Serial-port selection, and JPEG capture over Wi-Fi.
- Revised the electrical design through **HW03 and HW04**, confirming the OV5640 and the shared I2S audio signals.

The HW04 audio reference established **GPIO21 for BCLK, GPIO47 for WS, GPIO41 for microphone SD, and GPIO42 for amplifier DIN**. This September 6 milestone confirmed wiring, but **the microphone's RX driver had not yet been implemented in that revision**. Later versions added and validated microphone functions.

[Audio test](https://github.com/JuniorNarciso26/JrRobot/commit/6734fde4ec) · [HW04 wiring confirmation](https://github.com/JuniorNarciso26/JrRobot/commit/71f8adc172)

### September 7–8 — Early autonomy experiments and a safer Runtime API

Two important foundations appeared:

- **Local voice experiments:** autonomy controls, WakeNet/MultiNet recognition experiments, and local response behavior. These were development checkpoints, **not the release of a finished offline voice product**.
- **Runtime API v1.1:** a controlled interface for discovering capabilities, reading status and invoking a registered `face` action. It supported local HTTP/Serial control while avoiding arbitrary native-code or GPIO execution.

The Runtime API v1.1 line was integrated and promoted to `main` on **September 8**, alongside the I2S Philips framing correction.

[Runtime API 1.1 promotion](https://github.com/JuniorNarciso26/JrRobot/commit/76e0ab48b6089e8397c6bf304c242f85352253fb)

### September 13 — The first dedicated V1 control experience

The V1 workstream focused on controlling the robot through its **own browser interface**, rather than depending on the development panel.

- Introduced a minimal embedded web interface.
- Added camera controls and reception/playback of supported WAV audio.
- Established a clearer installer and development workflow.
- Recorded the firmware candidate identifier **`V1_03`**.

**Versioning note:** `V1_03` was a *technical firmware candidate identifier*. The history does not establish it as a separate public release named *JrBot V1.3*.

[Embedded portal work](https://github.com/JuniorNarciso26/JrRobot/commit/fc9f05a5a) · [Camera and WAV controls](https://github.com/JuniorNarciso26/JrRobot/commit/e2b824caf74ccdd9d1477b7004da84ce55afb046)

### September 13–14 — V1.4 and the refinements leading to V1.5

The next identifiable V1 milestone was **`JrBot_V1.4`**. Its commits improved camera behavior and the path for recording audio sent from a connected phone/computer.

The repository then recorded a succession of incremental firmware identifiers:

| Build | Documented evolution |
| --- | --- |
| **V1.4** | Camera and browser-to-robot audio improvements; firmware explicitly identified as V1.4 |
| **V1.4.1** | Fix for recording with the microphone without opening the camera |
| **V1.4.2** | Follow-up firmware identification and consistency corrections |
| **V1.4.3** | Expanded Wi-Fi/network diagnostics and related firmware identification |
| **V1.5** | New candidate identifier and preparation of the local HTTPS capability |

These are **recorded builds and candidates**; a commit naming a version is not, by itself, proof of a separate public release or full physical acceptance.

[V1.4 firmware identification](https://github.com/JuniorNarciso26/JrRobot/commit/d6cb5f7e2b2597c22a2c96d19fac618fac052966) · [V1.4.1 fix](https://github.com/JuniorNarciso26/JrRobot/commit/0b0fa36c13b91ff92517bd1d1f3699e827a557e) · [V1.5 candidate](https://github.com/JuniorNarciso26/JrRobot/commit/822848659357d67677f0637576cc2467c2559039)

### September 14 — V1.6 and V1.6.1: a usable local interface

This was the step that brought the earlier experiments together in a more complete browser-controlled JrBot.

- Added configurable camera resolutions, quality settings and continuous image handling.
- Integrated camera preview and **half-duplex push-to-talk (PTT) intercom** controls.
- Brought the internal portal and firmware identifiers into the V1.6 line.
- Consolidated **`JrBot_V1.6.1`** and standardized the reference camera orientation.

The V1.6.1 workstream was merged into `develop` on **September 14**. It became the starting point for the documented V1.6.2 changes.

[V1.6 camera and PTT integration](https://github.com/JuniorNarciso26/JrRobot/commit/cd8e42fc5b47d30595fa6369e648bd078aca63df) · [V1.6.1 merge](https://github.com/JuniorNarciso26/JrRobot/commit/33bed9aa3e7df465ce8a853bfc1e120f40a06cd9)

### September 19 — JrBot_V1.6.2: easier installation

The installer began discovering active remote branches dynamically, instead of relying on a fixed menu.

- Allowed selection of suitable `feature/*`, `fix/*`, and `hotfix/*` branches for testing.
- Hid `archive/*` branches from the normal selection workflow.
- Indicated the currently selected branch with `[ATUAL]`.
- Preserved the steps for updating, compiling, flashing and opening the development panel.

This release primarily improved the **development and installation experience**, rather than changing the robot's hardware or its existing PTT behavior.

### September 23 — JrBot_V1.6.3: real-time video and two-way audio

**Live WebRTC** changed the way people could interact with JrBot.

- Replaced the principal push-to-talk experience with **bidirectional live audio** using PCMA/G.711A at 8 kHz mono.
- Delivered OV5640 JPEG video through a WebRTC DataChannel/SCTP.
- Added local HTTPS signaling, independent audio-direction mute and video quality profiles.
- Validated simultaneous I2S reception/transmission on HW04, with BCLK and WS shared by microphone and amplifier.
- Improved PSRAM allocation to protect internal memory during ICE/DTLS/SCTP.

**Physical tests confirmed:** JrBot-to-phone and phone-to-JrBot sound, continuous video, and independent muting of both audio directions.

The earlier `JrBot_V1.6.2.1` camera-rotation hotfix was incorporated into this line, not promoted as a separate major product version. The repository also adopted the **MIT License** with documented third-party licensing boundaries.

**Limitations at this milestone:** acoustic echo cancellation was not implemented, WebRTC did not operate across the public Internet, and occasional video DataChannel backpressure remained observable.

### September 28 — JrBot_V1.7.04: a more reliable foundation

After adding live communication, the next priority was **coordination and reliability**.

- Removed repetitive automatic `/status` requests from the App.
- Introduced `jr_mode_manager` for explicit operating modes.
- Introduced `jr_resource_manager` to arbitrate I2S use between microphone, playback and Live.
- Added metrics for resource acquisition, release, busy events and ownership mismatches.
- Physically tested `IDLE → LIVE → IDLE` and the release of shared resources after a Live session.

**HW04 validation included:** recording, playback, full-duplex WebRTC, JPEG over DataChannel, I2S returning to owner `none`, `i2s_release_mismatch=0`, and no reboot or watchdog failure during the final recorded test.

**`JrBot_V1.7.04` became the last stable firmware version promoted to `main` in this history.** It provided the foundation for the next major integration.

Remaining improvements included AEC, video buffering, Internet-wide Live access and offline voice development in the separate V2 line.

## September–October 2026 — JrSkill Network adds a new dimension

### Colosseum 2026 — From fixed features to downloadable Skills

During the **Colosseum / Crypto World's Fair 2026 hackathon**, JrSkill Network was developed as an **upgrade to JrBot**, extending the robot beyond capabilities compiled directly into its firmware.

Its initial ecosystem connected:

1. **Skill creation and publishing:** structured, declarative JSON describing approved robot actions.
2. **JrSkill Store:** discovering Skills and acquiring licenses with a wallet.
3. **Solana Devnet:** Skill, Offer and License accounts supporting ownership and purchases in **test SOL**.
4. **My Skills:** discovering licensed Skills, selecting them and executing safe actions on the physical robot.

The work progressed from local JSON execution to an on-chain Skill PDA, retrieval and verification, wallet authentication, license purchases, and finally physical execution of a licensed Skill.

**October 7 — Physical execution validated:** the independently authored `jrteste` Skill ran on JrBot through the App and development panel. The reference firmware and panel were `JrBot_V1S_APP_03` and `JRBOT-PANEL-V1S-SKILLS-15`.

**October 10 — Initial milestone documented as complete:** the hackathon MVP had demonstrated the integrated Devnet acquisition, licensing and physical-execution journey. This is a **development milestone**, not a claim that a production Mainnet marketplace was launched.

The expanded creator/developer platform remains an ongoing area of development.

[Full Colosseum development diary — Kickoff and Day 1–9](../06-colosseum-history/HACKATHON_DEVLOG.md) · [Day 8 physical and transaction evidence](../06-colosseum-history/HACKATHON_DEVLOG/day8.md) · [Day 9 milestone summary](../06-colosseum-history/HACKATHON_DEVLOG/day9.md)

## Where this history stands

The next defined product version is **JrBot V1.8S**, bringing the existing JrBot abilities and JrSkill Network together. As documented here, the integrated release is still being prepared on `V1s-00`; promotion to `main` and consolidated-release testing must be recorded separately.

This Changelog preserves **the path already taken**. See the [Roadmap](ROADMAP.md) for the next steps and [Project Status](PROJECT_STATUS.md) for today's verified capabilities.
