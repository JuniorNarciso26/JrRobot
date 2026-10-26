# JrBot Project Status

**Last updated: October 26, 2026**

## Current status

**JrBot V1.8.1S is the current source version on `main`.** It builds on the V1.8S JrSkill integration and adds the RAM-light Wi-Fi work. Physical acceptance of this exact firmware remains to be recorded.

| Item | Status |
| --- | --- |
| **Current version** | `JrBot_V1.8.1S` |
| **Branch** | `main` |
| **Current work** | HW04 acceptance of Wi-Fi setup, mDNS, HTTPS/Live and memory |
| **Previous milestone** | V1.8S JrSkill integration |

## What V1.8.1S includes

JrBot combines its existing **camera, audio, expressive OLED face, Wi-Fi, browser controls, and local WebRTC Live** with **JrSkill Network**.

JrSkill enables users to discover, acquire, license, and execute approved Skills on a physical JrBot. Its initial end-to-end flow was validated on **Solana Devnet**, including physical execution. **Mainnet deployment is not part of this release status.**

The Wi-Fi setup access point opens when startup cannot connect for about 30 seconds, or when BOOT is held for three seconds. Its phone page lists nearby networks, tests the selected password and waits for an IP before saving. Saved networks remain visible on the page. Normal operation advertises `jrbot.local` over mDNS and retries mDNS initialization after a failure. Name resolution requires a local network and client that allow multicast DNS; it cannot work before the robot joins Wi-Fi.

## Next steps

1. Flash the exact V1.8.1S build and test correct and incorrect Wi-Fi passwords on HW04.
2. Verify `https://jrbot.local`, HTTPS/Live and memory on the local network.
3. Record the observed results in [Testing](../04-firmware-development/TESTING.md).

For previous versions, see the [Changelog](CHANGELOG.md). For future development, see the [Roadmap](ROADMAP.md).
