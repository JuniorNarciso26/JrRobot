# JrBot Project Status

**Last updated: October 26, 2026**

## Current status

**JrBot V1.8.1S is the current source version on `main`.** It builds on the V1.8S JrSkill integration and adds the RAM-light Wi-Fi work. On October 26, 2026, the project owner explicitly confirmed that physical testing of the current JrBot and memory tests were completed successfully. This is an owner-reported acceptance; detailed measurements and per-check logs were not supplied in this documentation update.

| Item | Status |
| --- | --- |
| **Current version** | `JrBot_V1.8.1S` |
| **Branch** | `main` |
| **Physical tests** | Completed / OK — owner confirmation on October 26, 2026 |
| **Memory tests** | Completed — owner confirmation on October 26, 2026 |
| **Current work** | Issue #26: secure remote access to panel and WebRTC, starting with an off-LAN connectivity proof; not yet implemented or validated remotely |
| **Previous milestone** | V1.8S JrSkill integration |

## What V1.8.1S includes

JrBot combines its existing **camera, audio, expressive OLED face, Wi-Fi, browser controls, and local WebRTC Live** with **JrSkill Network**.

JrSkill enables users to discover, acquire, license, and execute approved Skills on a physical JrBot. Its initial end-to-end flow was validated on **Solana Devnet**, including physical execution. **Mainnet deployment is not part of this release status.**

The Wi-Fi setup access point opens when startup cannot connect for about 30 seconds, or when BOOT is held for three seconds. Its phone page lists nearby networks, tests the selected password and waits for an IP before saving. Saved networks remain visible on the page. Normal operation advertises `jrbot.local` over mDNS and retries mDNS initialization after a failure. Name resolution requires a local network and client that allow multicast DNS; it cannot work before the robot joins Wi-Fi.

## Acceptance and next steps — October 26, 2026

The owner confirmed the physical tests **OK** and the memory tests **complete** and requested that they not be reopened for further data collection. This confirmation closes the current acceptance workstream for project management. The original build/RAM inventory remains documented separately; no new numerical measurements or per-scenario test logs are asserted here.

Following the Issue review, the next development workstream is [Issue #26 — secure remote access and WebRTC](https://github.com/JuniorNarciso26/JrRobot/issues/26). The local Live implementation is available and accepted; internet/off-LAN access is **not yet physically verified**. The first step is a private subnet-router/VPN experiment without firmware changes. Any later WebRTC media-routing or signaling changes will have their own acceptance and memory checks, without reopening the completed baseline tests. The Wi-Fi onboarding work is tracked in [Issue #55](https://github.com/JuniorNarciso26/JrRobot/issues/55) and the [Wi-Fi guide](../04-firmware-development/WIFI_PROVISIONING_RAM_LIGHT.md).

For previous versions, see the [Changelog](CHANGELOG.md). For future development, see the [Roadmap](ROADMAP.md).
