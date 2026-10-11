# JrBot — Testing and V1.8S Release Checks

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

The project has earlier **physically validated** camera, OLED, audio and local WebRTC milestones, and recorded Devnet-to-robot Skill execution in the Colosseum history. These are **not automatically a fresh consolidated V1.8S acceptance run**.

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
| **Wi-Fi/portal** | Configure via Serial, restart, access assigned IP | Local page and status reachable |
| **HTTPS** | Verify `JR_HTTPS_READY` | Secure server actually starts with local material |
| **Live WebRTC** | Connect on HTTPS, test video and two-way audio | Video and both audio directions observed on local network |
| **Resource manager** | Start/stop Live and repeat recording/playback | I2S released, no ownership mismatch or watchdog |
| **JrSkill** | Authenticate Devnet wallet and run a licensed Skill | Valid license, permitted actions and **visible physical execution** |
| **Recovery** | Test OLED missing/reconnection and service failures where safe | Expected error/retry behavior without unrelated failure |

**Do not claim HTTPS, browser-wallet or WebRTC passed unless certificate provisioning and actual secure-browser operation were tested.** Voice experiments must distinguish `brain_test` simulation from spoken recognition. Acoustic echo cancellation and public-Internet Live are **not verified release capabilities**.

## 5. V1.8S release gate

Before proposing a `V1s-00 → main` promotion:

1. Confirm the **exact source commit** and compiled firmware ID. The current files still report `JrBot_V1S_APP_03`; the named product milestone is **V1.8S**.
2. Address or explicitly document the **missing tracked HTTPS provisioning procedure** and confirm secure Live behavior.
3. Build and flash the consolidated candidate; record build logs and observed HW04 behavior.
4. Run relevant software tests and the physical checks above; write **pass/fail/not-run** for every item.
5. Record known limitations, developer sign-off and version references before explicitly approved promotion.

No new release check is claimed by this document.

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
