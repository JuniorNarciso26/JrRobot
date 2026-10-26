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

## 5. V1.8.1S verification status

The V1.8.1S candidate compiled with ESP-IDF 5.5.5 using
`sdkconfig.develop_wifi_ram_light`. The embedded RAM inventory was generated
and verified: **61,576 B DRAM**, **120,064 B IRAM**, **2,569,376 B application
image**. The pin-map check, HW04 pin-policy check and mDNS sdkconfig migration
test passed. The native-GCC mDNS behavior harness was skipped on this Windows
host because GCC is unavailable. Tests that require a real ESP32-S3, local
HTTPS material, a browser and a multicast-capable LAN remain **not run** until
their logs and observations are attached. A successful build does not prove
wrong-password rejection, mDNS resolution or Live.

Record **PASS / FAIL / NOT RUN** for every physical checklist item above.
If `jrbot.local` does not resolve on a particular LAN, check the mDNS log and
use the assigned IP to distinguish device failure from multicast filtering.

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
