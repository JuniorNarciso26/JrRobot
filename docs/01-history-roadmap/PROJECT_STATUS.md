# JrBot Project Status

**Last updated: November 7, 2026**

## Current status

**JrBot V1.8.2S is the current source version on `main`.** It retains the V1.8.1S RAM-light Wi-Fi work and adds the Mesa Mix for Live audio from [PR #57](https://github.com/JuniorNarciso26/JrRobot/pull/57). The owner accepted the working audio configuration for promotion after using it with headphones. V1.8.1S remains the earlier baseline for which the owner separately confirmed full physical and memory testing on October 26. Exact Audio Mix RAM/PSRAM measurements were not supplied.

| Item | Status |
| --- | --- |
| **Current version** | `JrBot_V1.8.2S` on `main` |
| **Previous baseline** | `JrBot_V1.8.1S` — owner-confirmed physical and memory testing on October 26 |
| **Live audio** | Mesa Mix with volume, microphone gain, echo ducking, noise gate and pre-buffer; owner reports good communication with headphones |
| **Memory evidence** | V1.8.1S baseline completed by owner report; no V1.8.2S RAM/PSRAM time series supplied |
| **Current work** | Delayed return of the user's own voice in Live; Issue #26 secure remote access proof, not yet validated off-LAN |
| **Previous milestone** | V1.8S JrSkill integration |

## What V1.8.1S includes

JrBot combines its existing **camera, audio, expressive OLED face, Wi-Fi, browser controls, and local WebRTC Live** with **JrSkill Network**.

JrSkill enables users to discover, acquire, license, and execute approved Skills on a physical JrBot. Its initial end-to-end flow was validated on **Solana Devnet**, including physical execution. **Mainnet deployment is not part of this release status.**

The Wi-Fi setup access point opens when startup cannot connect for about 30 seconds, or when BOOT is held for three seconds. Its phone page lists nearby networks, tests the selected password and waits for an IP before saving. Saved networks remain visible on the page. Normal operation advertises `jrbot.local` over mDNS and retries mDNS initialization after a failure. Name resolution requires a local network and client that allow multicast DNS; it cannot work before the robot joins Wi-Fi.

## V1.8.2S — Mesa Mix da Live

Mesa Mix in the robot's Live page adjusts WebRTC speaker volume, microphone gain, echo ducking, noise gate and pre-buffer without restarting the Live. The owner reports good communication with headphones and improved sound after lowering microphone gain and speaker volume while the microphone was open. Their own voice still returns with a delay even when away from the robot. Exact tuning values, latency measurements and V1.8.2S RAM/PSRAM time series were not provided; this release does not claim complete echo correction or measured long-session memory stability. See the [Mesa Mix guide](../04-firmware-development/LIVE_AUDIO_LAB.md) and [Changelog](CHANGELOG.md).

The exact V1.8.2S source passed a local ESP-IDF 5.5.5 RAM-light build, HW04 pin-policy checks and embedded JavaScript syntax validation on November 7. No new device was flashed for this validation.

## Acceptance and next steps — October 26, 2026

The owner confirmed the physical tests **OK** and the memory tests **complete** and requested that they not be reopened for further data collection. This confirmation closes the current acceptance workstream for project management. The original build/RAM inventory remains documented separately; no new numerical measurements or per-scenario test logs are asserted here.

Following the Issue review, the next development workstream is [Issue #26 — secure remote access and WebRTC](https://github.com/JuniorNarciso26/JrRobot/issues/26). The local Live implementation is available and accepted; internet/off-LAN access is **not yet physically verified**. The first step is a private subnet-router/VPN experiment without firmware changes. Any later WebRTC media-routing or signaling changes will have their own acceptance and memory checks, without reopening the completed baseline tests. The Wi-Fi onboarding work is tracked in [Issue #55](https://github.com/JuniorNarciso26/JrRobot/issues/55) and the [Wi-Fi guide](../04-firmware-development/WIFI_PROVISIONING_RAM_LIGHT.md).

For previous versions, see the [Changelog](CHANGELOG.md). For future development, see the [Roadmap](ROADMAP.md).
