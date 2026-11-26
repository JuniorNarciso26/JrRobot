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

**JrBot_V2.1** is the current release on `main`, approved by the owner on HW04.
It retains the established robot functions and adds offline pt-BR speech from
the HTTPS **Voz** tab with concurrent generation/playback of native vTTS blocks,
a cached “Olá ESP” greeting and the approved “Jay Ar Bot” name pronunciation.
Manual speech accepts up to 1200 UTF-8 bytes / 60 seconds of audio, depending on
memory; it uses fixed initial buffering, not unlimited-duration streaming.
See [Project Status](01-history-roadmap/PROJECT_STATUS.md),
[voice measurements](04-firmware-development/TTS_LONG_BENCHMARK.md) and
[Changelog](01-history-roadmap/CHANGELOG.md) for validation scope and limits.

This documentation follows the [MIT-licensed repository](../LICENSE). Historical material remains preserved in [trash/](../trash/README.md) for review; archived records are not necessarily up to date.
