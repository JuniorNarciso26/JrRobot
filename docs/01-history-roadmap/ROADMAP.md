# JrBot Roadmap

This roadmap summarizes **what has been delivered and what comes next**. For version-by-version details, see the [Changelog](CHANGELOG.md); for functionality available today, see [Project Status](PROJECT_STATUS.md).

## Documented development milestones

| Version | Main delivery | Status |
| --- | --- | --- |
| **V1.6.1** | Browser control, camera, audio, and push-to-talk intercom | Historical version |
| **V1.6.2** — Sep 19, 2026 | Installer with dynamic branch selection | Documented |
| **V1.6.3** — Sep 23, 2026 | Local Live WebRTC: bidirectional audio and JPEG video | Physically validated |
| **V1.7.04** — Sep 28, 2026 | Centralized mode and I2S resource management | **Latest stable release promoted to `main`** |
| **V1.8S** | Integration of existing robot capabilities with JrSkill Network | Integrated into `main` |
| **V1.8.1S** — Oct 26, 2026 | Lighter Wi-Fi firmware, phone setup, connection validation and `jrbot.local` recovery | In `main`; physical and memory testing confirmed complete by owner on Oct 26 |

During Colosseum 2026, JrSkill's creation, publication, license purchase, and physical execution flow was validated on **Solana Devnet**. The firmware used for that milestone identifies itself as `JrBot_V1S_APP_03`. V1.8S was later integrated into `main`; V1.8.1S adds the Wi-Fi and mDNS improvements described in the [Changelog](CHANGELOG.md). The original code promotion did not itself demonstrate acceptance. On October 26, 2026, the owner separately confirmed physical tests OK and memory tests complete; individual scenario logs and new measurements are not part of this note.

## Product roadmap governance — October 26, 2026

The original [roadmap Epic #17](https://github.com/JuniorNarciso26/JrRobot/issues/17) was **closed as completed on October 26, 2026**, because its planning and organizational purpose is now maintained in this document. The closing of the Epic **does not** mean all future versions have been delivered.

- **V1 (local browser control)**: accepted within the approved scope; [Issue #14](https://github.com/JuniorNarciso26/JrRobot/issues/14) closed.
- **V2 (local voice commands)**: future development tracked separately in [Issue #15](https://github.com/JuniorNarciso26/JrRobot/issues/15); revisit that Issue when V2 work actually resumes.
- **V3 (programmatic control)**: future scope remains in [Issue #16](https://github.com/JuniorNarciso26/JrRobot/issues/16).
- This roadmap is the single planning overview; open new, scoped implementation Issues when needed instead of keeping a duplicate roadmap Epic open.

## Development priorities

| Workstream | Verified starting point | Next objective |
| --- | --- | --- |
| **JrBot V1.8.1S** | Firmware build and host-level Wi-Fi/mDNS checks | Sustain completed acceptance; no reopening physical/memory tests |
| **Remote access / WebRTC — [Issue #26](https://github.com/JuniorNarciso26/JrRobot/issues/26)** | Local full-duplex audio and JPEG DataChannel Live working; current signaling is local only | Prioritize secure access from mobile networks: first prove private VPN reachability, then validate ICE/media; evaluate authenticated Internet signaling and TURN if needed, without exposing ESP32 ports |
| **JrSkill creator platform** | Initial Creator Studio, Store, licenses, and execution demonstrated on Devnet | Expand development and publishing tools for creators |
| **Hardware assembly and 3D-printed body** | Electronic assembly documented; mechanical development tracked in [Issue #22](https://github.com/JuniorNarciso26/JrRobot/issues/22) | Gather printable models, document body assembly, and validate the complete build |
| **JrBot V2** | Local voice-control development line preserved | Develop offline voice recognition and commands |
| **JrBot V3** | Existing Runtime API provides the technical foundation | Define the next programmatic-control product line |
| **JrBrain** | Long-term concept | Explore memory, context, personality, and AI integration |

Mechanical design and the JrSkill platform can progress in parallel with firmware work. **No confirmed delivery dates** are asserted for V2, V3, JrBrain, or a Mainnet marketplace.

## Proposed movement and actuator control

**Status: planning only — not implemented or electrically validated for motor driving.** The earlier HW04 pin review recorded candidate **control signals**, not a final motor wiring diagram:

| Candidate GPIO | Planned role | Constraint |
| ---: | --- | --- |
| **14** | Head movement control | Needs a validated actuator/driver design |
| **38** | Left-wheel movement control | Needs a validated actuator/driver design |
| **48** | Possible right-wheel control | Must first confirm the physical board does not use it for an onboard RGB LED or other function |

**GPIO39 and GPIO40 are avoided** because they belong to the JTAG/debug group and unexpected behavior was observed during tests. If GPIO48 is unavailable, evaluate an external motor driver or I/O expander instead of reusing Flash/PSRAM, USB, camera or audio pins.

No actuator, supply, motor-driver circuit or 3D body mount is approved by this proposal. **Do not power motors or servos directly from the ESP32-S3 3V3 rail.** Document and validate each part before moving it to the [HW04 Wiring Reference](../02-assembly/WIRING.md).

## Release-readiness checklist

1. Identify the firmware build, branch, and reference hardware.
2. Build the exact candidate intended for promotion.
3. Run applicable functional and physical tests.
4. Record results, known limitations, and remaining work.
5. Update this roadmap, the [Changelog](CHANGELOG.md), [Project Status](PROJECT_STATUS.md), and affected technical documentation.

A successful build is not equivalent to physical validation. Use the evidence levels defined in [Testing](../04-firmware-development/TESTING.md).

## Technical details and history

- [Current status and implemented capabilities](PROJECT_STATUS.md)
- [Hardware and assembly](../02-assembly/README.md)
- [JrSkill Network](../05-jrskill-network/README.md)
- [Colosseum 2026 development log](../06-colosseum-history/HACKATHON_DEVLOG.md)
