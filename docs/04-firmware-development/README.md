# JrBot — Firmware and Development

This directory explains **JrBot V1.8.3S on `main`**: how it is built, starts, communicates with clients and controls the physical robot. The Live audio controls from V1.8.2S are documented in [Mesa Mix](LIVE_AUDIO_LAB.md). The new **post-connection Wi-Fi tab** is documented in [Wi-Fi Dashboard](WIFI_DASHBOARD.md); first-run setup remains in [Wi-Fi Provisioning](WIFI_PROVISIONING_RAM_LIGHT.md). Release evidence and known limits are tracked in [Project Status](../01-history-roadmap/PROJECT_STATUS.md).

## Read in this order

| Document | What you will find |
| --- | --- |
| **[Development](DEVELOPMENT.md)** | Prerequisites, installation, Wi-Fi/HTTPS setup, build, flash and first run |
| **[Architecture](ARCHITECTURE.md)** | Real startup sequence, compiled modules, Live and JrSkill execution |
| **[Runtime API](API_RUNTIME.md)** | Serial commands, HTTP routes, supported API functions and errors |
| **[Testing](TESTING.md)** | How to test hardware, record evidence and prepare a release |
| **[Mesa Mix · Live](LIVE_AUDIO_LAB.md)** | V1.8.2S Live sound controls, RAM/PSRAM diagnostics and known audio limits |
| **[Wi-Fi Dashboard](WIFI_DASHBOARD.md)** | V1.8.3S signal/IP/channel, nearby Wi-Fi networks, up to four profiles, selection, deletion and confirmed reboot |

## Proposed features — not available as a general runtime engine

- [Flow Engine](FLOWS.md) — future trigger/condition/action automation.
- [Playground](PLAYGROUND.md) — future voice calibration and visual behavior authoring.

**Other references:** [HW04 Wiring](../02-assembly/WIRING.md) · [JrSkill Network](../05-jrskill-network/README.md) · [Changelog](../01-history-roadmap/CHANGELOG.md).

This section describes **observable behavior and compiled code**, not every historical source file. Archived designs remain in [`trash/`](../../trash/README.md).
