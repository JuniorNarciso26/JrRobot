# JrBot — Testing and V1.8.1S Release Checks

A **successful firmware build or positive API response is not enough** to claim that the physical robot worked. Every report must identify the tested branch/build and actual evidence.

## 1. Evidence levels

| Level | Meaning |
| --- | --- |
| **Planned** | Specification only |
| **Implemented** | Code is present but not necessarily build-tested |
| **Build verified** | Exact candidate compiles for the target |
| **Bench / simulated** | Host tests, mocks or logical tests only |
| **Physically validated** | Behavior observed on a real JrBot |

These labels describe **the tested scope**, not the status of every module in a firmware version.

## 2. Tests currently available in the repository

| Location | What the existing record supports |
| --- | --- |
| `tests/diagnostic/results.json` | 19 passing **host-mock/UBSan** diagnostic cases; explicitly **not** an ESP-IDF build or physical test |
| `tests/panel/results.json` | 33 Python and 13 browser passing cases in mocked/simulated setups; **not** a hardware validation |
| `tests/hardware/panel_results.json` | Explicitly `not_run` in the recorded GitHub edit session |
| `tests/hardware/test_pin_policy.py` | Pin-policy check used by the installer before building |

The project has earlier **physically validated** camera, OLED, audio and local WebRTC milestones, and recorded Devnet-to-robot Skill execution in the Colosseum history. These are **not automatically a fresh consolidated V1.8.1S acceptance run**.

## 3. Run the software checks

Before flashing a release candidate:

```bash
python tools/generate_pinmap.py --check
python tests/hardware/test_pin_policy.py
```

Then build the exact selected branch using `INSTALAR.bat build` as described in [Development](DEVELOPMENT.md). Save the build identifier and resulting log. If a separate test script is run, record **which script and environment**, rather than assuming archived results were rerun.

## 4. Physical validation checklist (HW04)

| Item | Action | Pass evidence |
| --- | --- | --- |
| **Boot** | Flash, reset and read `version`/`status` | Correct branch/build/profile, no unexpected restart |
| **OLED** | Issue a face command | Expression is visibly shown |
| **Camera** | `camera_test`, then `/capture` | Actual, viewable camera image |
| **Microphone** | `mic_test`, `/mic-record` | Input captured and heard in a recording |
| **Speaker** | `audio_test` or play a recording | Audible output, not just `ESP_OK` |
| **Wi-Fi setup** | Boot with no reachable saved network; join `JrBot-Setup-XXXXXX`, scan and enter a wrong password, then the correct one | Wrong password is rejected without saving; correct one gets IP, saves and restarts |
| **mDNS** | Rejoin the local network and open `https://jrbot.local` from a client on the same LAN | `JR_MDNS_READY`, name resolves and page opens; record `JR_MDNS_ERROR` if it retries |
| **HTTPS** | Verify `JR_HTTPS_READY` | Secure server actually starts with local material |
| **Live WebRTC** | Connect on HTTPS, test video and two-way audio | Video and both audio directions observed on local network |
| **Resource manager** | Start/stop Live and repeat recording/playback | I2S released, no ownership mismatch or watchdog |
| **JrSkill** | Authenticate Devnet wallet and run a licensed Skill | Valid license, permitted actions and **visible physical execution** |
| **Recovery** | Test OLED missing/reconnection and service failures where safe | Expected error/retry behavior without unrelated failure |

**Do not claim HTTPS, browser-wallet or WebRTC passed unless certificate provisioning and actual secure-browser operation were tested.** Voice experiments must distinguish `brain_test` simulation from spoken recognition. Acoustic echo cancellation and public-Internet Live are **not verified release capabilities**.

## 5. V1.8.1S verification status — updated October 26, 2026

**Owner-reported final acceptance:** the project owner explicitly confirmed that the **physical tests are OK** and **memory tests are completed**, and that no additional memory/test information is required for this milestone. Treat these two workstreams as complete. This is a direct owner confirmation, not a new test performed by this documentation change. It does not supply per-item logs, exact observation times, or new RAM measurements; do not invent them.

**Previously recorded build evidence (retained):** the V1.8.1S candidate compiled with ESP-IDF 5.5.5 using `sdkconfig.develop_wifi_ram_light`. The embedded static RAM inventory was **61,576 B DRAM**, **120,064 B IRAM**, **2,569,376 B application image**. Pin-map, HW04 pin-policy and mDNS sdkconfig migration checks passed. The native-GCC mDNS behavior harness was previously skipped on a Windows host without GCC.

The archived `tests/hardware/panel_results.json` and older notes reflect their own historical test sessions and may continue to say `not_run`. They are not overwritten or retroactively represented as evidence for the October 26 acceptance. This status update does not newly validate AEC, cloud Internet Live, or any proposed future feature.

If future releases or regressions require a new acceptance campaign, collect **PASS / FAIL / NOT RUN** by scenario and preserve the corresponding logs. If `jrbot.local` fails on a specific LAN, distinguish mDNS filtering from an IP connectivity failure.

## 6. Minimum reproducible report

```text
Date / tester:
Branch and commit:
Firmware ID:
ESP-IDF version:
Hardware revision:
Connection (Serial, HTTP, HTTPS):
Test and steps:
Expected result:
Observed result:
Evidence (logs, media):
Result (PASS / FAIL / NOT RUN):
Known limitations / issue:
```

For the **current state**, consult [Project Status](../01-history-roadmap/PROJECT_STATUS.md). For historic validations, consult [Changelog](../01-history-roadmap/CHANGELOG.md) and [Colosseum Devlog](../06-colosseum-history/HACKATHON_DEVLOG.md).
