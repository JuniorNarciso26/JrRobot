# JrBot Project Status

**Last updated: November 7, 2026**

## Current status

**JrBot V1.8.1S is the last owner-accepted baseline.** It builds on the V1.8S JrSkill integration and adds the RAM-light Wi-Fi work. On October 26, 2026, the project owner explicitly confirmed that physical and memory tests were completed successfully. This is an owner-reported acceptance; detailed measurements and per-check logs were not supplied. The later `AUDIO_LAB_01` candidate is experimental and has separate validation limits below.

| Item | Status |
| --- | --- |
| **Accepted baseline** | `JrBot_V1.8.1S` |
| **Experimental candidate** | `JrBot_V1.8.1S_AUDIO_LAB_01` — Live Audio Lab, [PR #57](https://github.com/JuniorNarciso26/JrRobot/pull/57) |
| **Physical tests** | Baseline completed / OK — owner confirmation on October 26; Audio Lab communication reported working with headphones on November 7 |
| **Memory tests** | Baseline completed — owner confirmation on October 26; Audio Lab RAM/PSRAM measurements not yet supplied |
| **Current work** | Delayed return of the user's own voice in Audio Lab; Issue #26 secure remote access proof, not yet validated off-LAN |
| **Previous milestone** | V1.8S JrSkill integration |

## What V1.8.1S includes

JrBot combines its existing **camera, audio, expressive OLED face, Wi-Fi, browser controls, and local WebRTC Live** with **JrSkill Network**.

JrSkill enables users to discover, acquire, license, and execute approved Skills on a physical JrBot. Its initial end-to-end flow was validated on **Solana Devnet**, including physical execution. **Mainnet deployment is not part of this release status.**

The Wi-Fi setup access point opens when startup cannot connect for about 30 seconds, or when BOOT is held for three seconds. Its phone page lists nearby networks, tests the selected password and waits for an IP before saving. Saved networks remain visible on the page. Normal operation advertises `jrbot.local` over mDNS and retries mDNS initialization after a failure. Name resolution requires a local network and client that allow multicast DNS; it cannot work before the robot joins Wi-Fi.

## Experimental Live Audio Lab — November 7, 2026

The Mesa de Áudio in the robot's Live page adjusts WebRTC speaker volume, microphone gain, echo ducking, noise gate and pre-buffer without a reboot. The owner reports that communication works and sounds good with headphones, and that lowering microphone gain and speaker volume while the microphone is open improves sound. Their own voice still returns with a delay even when away from the robot. Exact tuning values, latency measurements and Audio Lab RAM/PSRAM time series were not provided; no claim of complete echo correction or memory stability is made. See the [Audio Lab guide](../04-firmware-development/LIVE_AUDIO_LAB.md) and [Changelog](CHANGELOG.md).

The PR #57 source passed a local ESP-IDF 5.5.5 RAM-light build, HW04 pin-policy checks and embedded JavaScript syntax validation on November 7. No new device was flashed for this validation.

## Acceptance and next steps — October 26, 2026

The owner confirmed the physical tests **OK** and the memory tests **complete** and requested that they not be reopened for further data collection. This confirmation closes the current acceptance workstream for project management. The original build/RAM inventory remains documented separately; no new numerical measurements or per-scenario test logs are asserted here.

Following the Issue review, the next development workstream is [Issue #26 — secure remote access and WebRTC](https://github.com/JuniorNarciso26/JrRobot/issues/26). The local Live implementation is available and accepted; internet/off-LAN access is **not yet physically verified**. The first step is a private subnet-router/VPN experiment without firmware changes. Any later WebRTC media-routing or signaling changes will have their own acceptance and memory checks, without reopening the completed baseline tests. The Wi-Fi onboarding work is tracked in [Issue #55](https://github.com/JuniorNarciso26/JrRobot/issues/55) and the [Wi-Fi guide](../04-firmware-development/WIFI_PROVISIONING_RAM_LIGHT.md).

For previous versions, see the [Changelog](CHANGELOG.md). For future development, see the [Roadmap](ROADMAP.md).
