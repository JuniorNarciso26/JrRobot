<div align="center">

<img src="docs/assets/readme/jrbot-logo.svg" alt="JrBot logo" width="360">

# JrBot

**Open-source Physical AI Robot powered by ESP32-S3**  
See. Hear. Speak. Connect. Express.

<img src="https://img.shields.io/github/license/JuniorNarciso26/JrRobot" alt="MIT License">
<img src="https://img.shields.io/badge/ESP32--S3-ESP--IDF-red" alt="ESP32-S3">
<img src="https://img.shields.io/badge/WebRTC-local%20live-blue" alt="WebRTC">
<img src="https://img.shields.io/badge/stable-JrBot__V1.7.04-brightgreen" alt="Stable release">
<img src="https://img.shields.io/github/stars/JuniorNarciso26/JrRobot?style=social" alt="GitHub stars">

[Website](https://jrbot.com.br) ·
[Build your own](#build-your-own-jrbot) ·
[JrSkill Network](#jrskill-network--colosseum-2026-mvp-achieved) ·
[Documentation](docs/README.md) ·
[Roadmap](docs/ROADMAP.md) ·
[Contribute](CONTRIBUTING.md)

</div>

<p align="center">
  <img src="docs/assets/readme/jrbot-demo.gif" alt="JrBot physical robot demo" width="360">
</p>

## AI is leaving the screen.

Most AI assistants live inside a browser, phone or computer. **JrBot explores what happens when AI has a physical presence.**

JrBot is an open-source ESP32-S3 robot with a camera, microphone, speaker, expressive OLED face, local connectivity and real-time audio/video. Its hardware foundation supports future voice, intelligence and portable Skills.

> **JrBot was not created only to answer. It was created to be there.**

## What JrBot does

| Capability | Implemented features |
| --- | --- |
| **See** | OV5640 photos, browser preview and Live JPEG video; QVGA/VGA/SVGA, image quality and FPS controls |
| **Hear** | MS3625 I2S recording, selectable duration, WAV playback/download and bidirectional WebRTC audio |
| **Speak** | MAX98357A output, speaker playback, audio received from phone/computer, volume and diagnostics |
| **Express** | SSD1306 OLED face with selectable expressions and active-expression status |
| **Connect** | Wi-Fi (DHCP, saved configuration and reconnection), responsive web interface, local HTTPS and WebRTC |
| **Interact** | Direct browser control on the local network, including Live with independent audio-direction mute |

These capabilities form the **physically validated local-control baseline**, not a claim that every future AI feature is already available.

### Verified firmware baselines

- **`JrBot_V1.7.04` — stable local-control release (HW04).** Microphone, speaker, OLED, camera and local WebRTC Live were physically validated. The hardware uses shared I2S BCLK/WS for simultaneous RX/TX, with `jr_mode_manager` and `jr_resource_manager` managing mode and I2S ownership. The final recorded V1.7.04 test returned the resource owner to `none` without reset or watchdog.
- **`JrBot_V1S_APP_03` — JrSkill / Solana Devnet MVP.** A separately tested firmware line added licensed Skill discovery, selection and constrained physical execution. Its second-Skill hardware test was confirmed on 2026-10-07; this does **not** constitute a new stable-release certification or a Solana Mainnet launch.

**Known boundaries:** Live operates on the local network; Internet WebRTC, STUN/TURN and remote signaling are future work. Further AEC/echo and video-backpressure improvements remain separate engineering work. See [Testing](docs/TESTING.md) and [Project status](docs/PROJECT_STATUS.md).

## Build your own JrBot

JrBot is designed to be built, modified and extended.

### HW04 reference hardware

| Component | Role |
| --- | --- |
| **ESP32-S3 N16R8** | Main controller |
| **OV5640** | Camera; validated orientation -90° / 270° |
| **SSD1306 OLED** | Face and expressions |
| **MS3625 I2S** | Microphone |
| **MAX98357A** | Audio amplifier and speaker output |

The microphone and amplifier share the following I2S clock signals:

| Signal | GPIO |
| --- | ---: |
| BCLK / SCK | 21 |
| WS / LRCLK | 47 |
| MS3625 SD | 41 |
| MAX98357A DIN | 42 |

For the remaining hardware details, see [Pin mapping](docs/PINAGEM.md) and the [wiring diagram](docs/ESQUEMA_LIGACAO.md).

### Firmware installation (Windows)

1. Clone or download the repository and connect the ESP32-S3 via USB.
2. Run **`INSTALAR.bat`** from the repository root.
3. Select the intended branch in the installer, then compile and flash.
4. Confirm the actual firmware version in the device log/status.

The installer allows choosing between available branches; **check the firmware version rather than assuming every branch uses the same build**. Development and validation instructions are in [DEVELOPMENT.md](docs/DEVELOPMENT.md).

## JrSkill Network — Colosseum 2026 MVP achieved

In nearly two weeks of hackathon development, we built an entirely new **Solana-powered Skill economy layer** on top of the pre-existing JrBot platform. The **Colosseum / Crypto World's Fair 2026 MVP was achieved**: Skill creation, publication, wallet-based purchasing, license verification and execution on a physical robot worked together on **Solana Devnet**.

### One end-to-end ecosystem

- **Creator Studio and bilingual JrSkill Store** (`JrBot_Web`): authors create a declarative JSON Skill, publish an Offer and make it available to other wallets.
- **Store API** (`JrBot_Cloud`): HTTPS backend for transaction validation, publication and license-state checks.
- **Solana Devnet**: Skill, Offer and License accounts; Phantom / Wallet Standard integration and test-SOL payments.
- **JrBot My Skills App** (`JrRobot`): authenticated wallet, licensed Skill discovery and selection, followed by approved robot operations.

~~~text
Creator Studio -> Skill JSON v1
        |
JrSkill Store -> Publish Skill + Offer
        |
Solana Devnet -> Phantom purchase + License
        |
My Skills App -> Verify + select licensed Skill
        |
Controlled Skill Executor / Runtime API
        |
Physical JrBot -> Authorized behavior
~~~

### What the MVP proved

The reference Skill demonstrated the original store-to-robot path. We then created a second, independently authored Skill, **`jrteste`**, through the website. Another Phantom wallet purchased its license, and `JrBot_V1S_APP_03` executed it on real hardware with `result=ok`, as confirmed by the maintainer on 2026-10-07.

The demonstrated **1 test SOL** purchase allocated **0.5 test SOL to the creator and 0.5 test SOL to the JrBot treasury**. This validates the proposed revenue-sharing mechanism in a test environment, not real Mainnet revenue.

**Security boundary:** the physically validated MVP limits declarative Skill execution to allowlisted OLED expressions (`face`) and delays (`wait`). Blockchain data and third-party Skills cannot directly execute arbitrary native code, shell commands or raw GPIO operations on the ESP32. Broader capabilities, a production marketplace and further security validation remain future work.

### From an MVP to a global Skill economy

The proven architecture is a foundation for a possible worldwide marketplace where creators can publish reusable robot capabilities, offer verifiable licenses and reach robot owners across borders. **The V1S hackathon milestone is complete; the JrSkill Network vision continues.** Global commercial operation, Mainnet deployment and external adoption are not claimed.

**Explore the evidence and project:**

- [Hackathon development log — Kickoff and Day 1–9](docs/HACKATHON_DEVLOG.md)
- [Day 8 — transactions, screenshots and physical validation](docs/HACKATHON_DEVLOG/day8.md)
- [Day 9 — MVP victory and global vision](docs/HACKATHON_DEVLOG/day9.md)
- [JrSkill Execution API](docs/JRSKILL_API.md) · [My Skills App](docs/JRSKILL_APP.md)
- [JrSkill Store](https://jrbot.com.br/en/store/) · [Colosseum project](https://colosseum.com/arena/projects/jrskill-network) · [Issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33)

## Project status and roadmap

| Project line | Current status |
| --- | --- |
| **V1 — local control** | **`JrBot_V1.7.04`**, stable and physically validated |
| **V1S — JrSkill / Solana** | **`JrBot_V1S_APP_03`**, Devnet MVP achieved on `V1s-00`; integration candidate |
| **V2 — local voice** | Separate development line |
| **V3 — programmatic control** | Future product line built on Runtime API |
| **JrBrain** | Future memory, personality, context, LLM and Skills |

The `main` branch remains the reference for officially promoted releases; `V1s-00` preserves the complete Colosseum development history. **The next JrSkill development line will be planned separately.** See the [official roadmap](docs/ROADMAP.md) for the longer-term direction.

## Contributing and documentation

JrBot welcomes contributions in ESP32-S3 firmware, hardware, audio/WebRTC, camera, frontend, AI, Solana, testing and documentation. You do not need to own a complete robot to contribute.

Start with the [Contribution Guide](CONTRIBUTING.md) and [open Issues](https://github.com/JuniorNarciso26/JrRobot/issues). Describe the change, evidence and any missing physical tests in a Pull Request.

**Documentation:** [Index](docs/README.md) · [Architecture](docs/ARCHITECTURE.md) · [Development](docs/DEVELOPMENT.md) · [Testing](docs/TESTING.md) · [Runtime API](docs/API_RUNTIME.md) · [Changelog](docs/CHANGELOG.md)

## License

Original repository content is released under the [MIT License](LICENSE), unless explicitly stated otherwise. Third-party components retain their own licenses. The MIT License does not grant rights to the **JrBot** trademark or authorize presenting derivative products as official JrBot products.

## Vision

**JrBot connects physical presence with a future ecosystem of portable abilities.** The goal is a robot that can listen, perceive, respond and gain new Skills without losing control of its hardware.

<p align="center">
  <strong>JrBot</strong><br>
  Listen. Understand. Respond. React. Express.<br><br>
  <em>Project by JrTk Invest</em>
</p>

---

<p align="center">
  If you want to follow the evolution of open-source physical AI, consider starring the repository.
</p>
