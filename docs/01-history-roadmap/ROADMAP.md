# JrBot Roadmap

This roadmap summarizes **what has been delivered and what comes next**. For version-by-version details, see the [Changelog](CHANGELOG.md); for functionality available today, see [Project Status](PROJECT_STATUS.md).

## Documented development milestones

| Version | Main delivery | Status |
| --- | --- | --- |
| **V1.6.1** | Browser control, camera, audio, and push-to-talk intercom | Historical version |
| **V1.6.2** — Sep 19, 2026 | Installer with dynamic branch selection | Documented |
| **V1.6.3** — Sep 23, 2026 | Local Live WebRTC: bidirectional audio and JPEG video | Physically validated |
| **V1.7.04** — Sep 28, 2026 | Centralized mode and I2S resource management | **Latest stable release promoted to `main`** |
| **V1.8S** — next defined release | Integration of existing robot capabilities with JrSkill Network | **Integration prepared in `V1s-00`; promotion pending** |

During Colosseum 2026, JrSkill's creation, publication, license purchase, and physical execution flow was validated on **Solana Devnet**. The firmware used for this milestone identifies itself as `JrBot_V1S_APP_03`. **V1.8S is the intended next product version**, not a claim that the `main` branch has already been released under that name.

## Development priorities

| Workstream | Verified starting point | Next objective |
| --- | --- | --- |
| **JrBot V1.8S** | Existing robot features and JrSkill flow validated within their documented scopes | Verify the consolidated release, align the firmware version, and promote to `main` |
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
