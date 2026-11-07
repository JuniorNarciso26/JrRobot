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

**JrBot V1.8.2S** is the current source version on `main`. It retains the V1.8.1S Wi-Fi setup and adds the [Mesa Mix for Live audio](04-firmware-development/LIVE_AUDIO_LAB.md): speaker volume, microphone gain, echo ducking, noise gate, pre-buffer and RAM/PSRAM diagnostics. The reported delayed return of the user's own voice remains a known limitation. See [Project Status](01-history-roadmap/PROJECT_STATUS.md) for the evidence and its limits.

This documentation follows the [MIT-licensed repository](../LICENSE). Historical material remains preserved in [trash/](../trash/README.md) for review; archived records are not necessarily up to date.
