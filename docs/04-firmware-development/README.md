# JrBot — Firmware and Development

This directory explains how the **current firmware on `V1s-00`** is built, starts, communicates with clients and controls the physical robot. JrBot V1.8S development is recorded as complete; **final release checks and promotion to `main` are still pending**. See [Project Status](../01-history-roadmap/PROJECT_STATUS.md).

## Read in this order

| Document | What you will find |
| --- | --- |
| **[Development](DEVELOPMENT.md)** | Prerequisites, installation, Wi-Fi/HTTPS setup, build, flash and first run |
| **[Architecture](ARCHITECTURE.md)** | Real startup sequence, compiled modules, Live and JrSkill execution |
| **[Runtime API](API_RUNTIME.md)** | Serial commands, HTTP routes, supported API functions and errors |
| **[Testing](TESTING.md)** | How to test hardware, record evidence and prepare a release |

## Proposed features — not available as a general runtime engine

- [Flow Engine](FLOWS.md) — future trigger/condition/action automation.
- [Playground](PLAYGROUND.md) — future voice calibration and visual behavior authoring.

**Other references:** [HW04 Wiring](../02-assembly/WIRING.md) · [JrSkill Network](../05-jrskill-network/README.md) · [Changelog](../01-history-roadmap/CHANGELOG.md).

This section describes **observable behavior and compiled code**, not every historical source file. Archived designs remain in [`trash/`](../../trash/README.md).
