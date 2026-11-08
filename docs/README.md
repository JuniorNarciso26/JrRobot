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

**JrBot V1.8.3S** is the current source version on `main`. It includes the [Mesa Mix for Live audio](04-firmware-development/LIVE_AUDIO_LAB.md) from V1.8.2S and adds the [Wi-Fi dashboard](04-firmware-development/WIFI_DASHBOARD.md): current SSID/IP/channel/RSSI, optional signal updates, manual network scanning, up to four saved profiles, add/select/delete with confirmation, and confirmed reboot to switch networks. The existing phone setup is retained. The delayed return of the user's voice remains a known audio limitation; authentication for local Wi-Fi management and full V1.8.3S hardware/memory regression evidence are not claimed. See [Project Status](01-history-roadmap/PROJECT_STATUS.md).

This documentation follows the [MIT-licensed repository](../LICENSE). Historical material remains preserved in [trash/](../trash/README.md) for review; archived records are not necessarily up to date.
