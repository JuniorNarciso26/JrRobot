# JrBot Documentation

This documentation is organized by **primary topic**, so readers can understand the robot, assemble its hardware, develop its firmware, and explore the JrSkill ecosystem without navigating the hackathon records first.

## Documentation sections

1. **[History and Roadmap](01-history-roadmap/README.md)** — project introduction, documented versions, current status, and future plans.
2. **[Hardware Assembly](02-assembly/README.md)** — components, pin mapping, electrical connections, and reference hardware.
3. **[3D Printing and Body](03-3d-printing/README.md)** — mechanical design and body assembly; complete printable models and instructions still need to be gathered.
4. **[Firmware and Development](04-firmware-development/README.md)** — architecture, setup, Runtime API, and testing.
5. **[JrSkill Network](05-jrskill-network/README.md)** — Skills, licensing, Solana Devnet, and controlled physical execution.
6. **[Colosseum 2026 History](06-colosseum-history/README.md)** — preserved Kickoff and Day 1–9 development logs and evidence.

## Version status

**JrBot V2.0** is the current source version on `main`. It retains the features developed through V1.8.4S (Wi-Fi reliability and dashboard, Mesa Mix, local WebRTC, camera, OLED and JrSkill Network) and introduces offline Portuguese wake-word detection using WakeNet10 `wn10_pt_olaesp`. In autonomous mode, “Olá ESP” causes a beep, followed by the **Love** face for three seconds and automatic return to Neutral. The existing panel and mobile `jrbot.local` page can both switch autonomous mode on or off. The owner tested and approved the V2.0 scope on real HW04 hardware; free-form speech and TTS remain upcoming work. See [V2 integration](04-firmware-development/ESP_SR_OLA_ESP_AUTONOMO.md) and [Project Status](01-history-roadmap/PROJECT_STATUS.md).

This documentation follows the [MIT-licensed repository](../LICENSE). Historical material remains preserved in [trash/](../trash/README.md) for review; archived records are not necessarily up to date.
