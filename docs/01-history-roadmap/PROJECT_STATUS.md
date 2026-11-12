# JrBot Project Status

**Last updated: November 8, 2026**

## Candidate under test — V1.8.4S (`develop`)

The `develop` branch has been synchronized with the approved V1.8.3S
`main` tree and now carries a candidate Wi-Fi reliability fix. It tries
the selected Wi-Fi first, then other saved profiles after bounded failures,
and remembers a successful alternative for the next boot. A follow-up HW04
log showed captive setup opening before the first fallback finished; the
candidate now attempts one automatic software restart on startup timeout,
then opens setup if the subsequent boot also fails (no reboot loop). This is not a
V1.8.4S production release: ESP-IDF build and HW04 tests, Live regressions
and RAM/PSRAM measurements remain to be completed. `main` stays at V1.8.3S.

## Current status

**JrBot V1.8.3S is the current source version on `main`.** It retains the V1.8.2S Mesa Mix for Live audio from [PR #57](https://github.com/JuniorNarciso26/JrRobot/pull/57) and adds the connected-mode Wi-Fi dashboard from [PR #59](https://github.com/JuniorNarciso26/JrRobot/pull/59): current SSID, IP, RSSI, channel and mode; manual scan; up to four saved profiles; add/select/delete; and confirmed reboot for switching networks. The owner approved its promotion to `main`. That acceptance is not, by itself, an independently logged full hardware regression or measured RAM/PSRAM test. V1.8.1S remains the earlier baseline for which the owner separately confirmed full physical and memory testing on October 26.

| Item | Status |
| --- | --- |
| **Current version** | `JrBot_V1.8.3S` on `main` (Wi-Fi management dashboard; previous V1.8.2S Mesa Mix retained) |
| **Previous baseline** | `JrBot_V1.8.1S` — owner-confirmed physical and memory testing on October 26 |
| **Live audio** | Mesa Mix with volume, microphone gain, echo ducking, noise gate and pre-buffer; owner reports good communication with headphones |
| **Wi-Fi management** | Connected-mode Wi-Fi tab: RSSI, IP, SSID, channel, scan while idle, up to four profiles, add/select/delete, reboot confirmation; no hot switching |
| **Memory evidence** | V1.8.1S baseline completed by owner report; no separate V1.8.3S long-session RAM/PSRAM series supplied |
| **Current work** | Delayed return of the user's own voice in Live; Issue #26 secure remote access proof, not yet validated off-LAN |
| **Previous milestone** | V1.8S JrSkill integration |

## What V1.8.1S includes

JrBot combines its existing **camera, audio, expressive OLED face, Wi-Fi, browser controls, and local WebRTC Live** with **JrSkill Network**.

JrSkill enables users to discover, acquire, license, and execute approved Skills on a physical JrBot. Its initial end-to-end flow was validated on **Solana Devnet**, including physical execution. **Mainnet deployment is not part of this release status.**

The Wi-Fi setup access point opens when startup cannot connect for about 30 seconds, or when BOOT is held for three seconds. Its phone page lists nearby networks, tests the selected password and waits for an IP before saving. Saved networks remain visible on the page. Normal operation advertises `jrbot.local` over mDNS and retries mDNS initialization after a failure. Name resolution requires a local network and client that allow multicast DNS; it cannot work before the robot joins Wi-Fi.

## V1.8.2S — Mesa Mix da Live

Mesa Mix in the robot's Live page adjusts WebRTC speaker volume, microphone gain, echo ducking, noise gate and pre-buffer without restarting the Live. The owner reports good communication with headphones and improved sound after lowering microphone gain and speaker volume while the microphone was open. Their own voice still returns with a delay even when away from the robot. Exact tuning values, latency measurements and V1.8.2S RAM/PSRAM time series were not provided; this release does not claim complete echo correction or measured long-session memory stability. See the [Mesa Mix guide](../04-firmware-development/LIVE_AUDIO_LAB.md) and [Changelog](CHANGELOG.md).

The exact V1.8.2S source passed a local ESP-IDF 5.5.5 RAM-light build, HW04 pin-policy checks and embedded JavaScript syntax validation on November 7. No new device was flashed for this validation.

## V1.8.3S — Wi-Fi management after onboarding

The portal `https://jrbot.local` now presents **Control, Memory and Wi-Fi** tabs. Wi-Fi status includes connected/disconnected state, SSID, assigned IPv4 address, Wi-Fi channel, operating mode and RSSI in dBm; the signal can be refreshed manually or automatically every 10 seconds while the Wi-Fi tab remains visible. The on-demand scan displays up to eight nearby networks and is blocked during Live or autonomous operation.

Users may store **up to four network profiles**, select one for the next boot, and remove a profile using **Excluir / Delete**, with a confirmation prompt. Deleting the selected profile requires an additional warning because the robot may need initial setup on its next reboot. Saving/selecting does **not** immediately disconnect the active station; applying the selected network requires a separate confirmed reboot. The existing initial provisioning AP and USB/Serial setup remain available.

The dashboard is implemented in `firmware/main/wifi_panel.js` and its HTTPS-only API in `firmware/module_portal/jr_wifi_web.c`; it reuses the existing NVS profile store and manual scanner. There is **no dedicated background scan task** and no new continuous monitoring on the ESP32-S3. The endpoint has browser-origin checks but no user authentication: this is **LAN-only management, not Internet-safe remote administration**. For use and safeguards, see [Wi-Fi Dashboard](../04-firmware-development/WIFI_DASHBOARD.md); for protocol details, see [Runtime API](../04-firmware-development/API_RUNTIME.md).

**Evidence:** PR #59 was merged and the firmware ID `JrBot_V1.8.3S` was verified on `main`. The owner accepted the UI and closed the Wi-Fi feature workstream. Documentation does not assert a complete independently observed ESP-IDF build, network-switch recovery exercise, or live memory profile for V1.8.3S.

## Acceptance and next steps — October 26, 2026

The owner confirmed the physical tests **OK** and the memory tests **complete** and requested that they not be reopened for further data collection. This confirmation closes the current acceptance workstream for project management. The original build/RAM inventory remains documented separately; no new numerical measurements or per-scenario test logs are asserted here.

Following the Issue review, the next development workstream is [Issue #26 — secure remote access and WebRTC](https://github.com/JuniorNarciso26/JrRobot/issues/26). The local Live implementation is available and accepted; internet/off-LAN access is **not yet physically verified**. The first step is a private subnet-router/VPN experiment without firmware changes. Any later WebRTC media-routing or signaling changes will have their own acceptance and memory checks, without reopening the completed baseline tests. The Wi-Fi onboarding work is tracked in [Issue #55](https://github.com/JuniorNarciso26/JrRobot/issues/55) and the [Wi-Fi guide](../04-firmware-development/WIFI_PROVISIONING_RAM_LIGHT.md).

For previous versions, see the [Changelog](CHANGELOG.md). For future development, see the [Roadmap](ROADMAP.md).
