# JrBot — Firmware Architecture and Execution

This document follows the **actual ESP-IDF source compiled from `V1s-00`**. It explains what starts on the ESP32-S3 and how commands reach hardware. Installation belongs to [Development](DEVELOPMENT.md); command formats belong to [Runtime API](API_RUNTIME.md).

## 1. Firmware layout and build

`firmware/CMakeLists.txt` selects the `esp32s3` target; `firmware/main/CMakeLists.txt` names the C files included in the firmware and embeds `jrskill_app.js` as a text asset.

| Code path | Responsibility |
| --- | --- |
| `main/main.c` | `app_main()` and boot loop |
| `core/jr_wifi.c` | Wi-Fi, NVS settings and connection status |
| `core/jr_commands.c`, `jr_usb_terminal.c` | Serial/USB commands and status |
| `core/jr_mode_manager.c` | Logical operating modes |
| `core/jr_resource_manager.c` | Shared I2S ownership and diagnostics |
| `module_face/jr_face.c` | SSD1306 expressions and refresh |
| `module_camera/jr_camera_diag.c` | **Active** OV5640 capture and JPEG diagnostics |
| `module_audio/jr_mic.c`, `jr_audio.c` | Microphone and speaker on demand |
| `module_audio/jr_webrtc_audio.c` | Local Live WebRTC audio and JPEG transport |
| `module_portal/jr_portal.c`, `jr_audio_receive.c` | Local HTTP/HTTPS routes, browser assets and WAV reception |
| `module_runtime/jr_runtime_api.c` | Allowlisted JSON API |
| `module_brain/jr_brain.c`, `jr_voice.c`, `jr_reply_audio.c` | Local voice/brain experiments; not a finished V2 voice release |
| `main/jrskill_app.js` | **Browser-side** JrSkill App embedded in the firmware image |

**Camera implementation:** `module_camera/jr_camera.c` and its header remain in the repository, but **`jr_camera.c` is not listed in the current firmware CMake sources**. For live firmware changes use `jr_camera_diag.c`; do not assume an uncompiled source file is active.

Some CMake/Kconfig labels still contain `jrbot_v2` or `JrBot V2`. They are **legacy internal names**, not evidence that the experimental V2 voice release was promoted.

## 2. Boot sequence in `main.c`

```text
ESP-IDF boot → app_main()
     ↓
Log firmware version, hardware profile and module pins
     ↓
jr_wifi_prepare()       (load Wi-Fi settings from NVS/defaults)
jr_commands_init()     (I2S resource manager and command mutex)
     ↓
Start Serial/UART and USB Serial/JTAG command terminals
     ↓
jr_face_start()         (initialize OLED; failure does not stop boot)
     ↓
jr_wifi_start()         (station connection attempt)
     ↓
If network stack ready: jr_portal_start() (HTTP and conditional HTTPS)
     ↓
Main loop: update OLED expressions and periodically retry an offline OLED
```

**Important distinction:** `jr_wifi_network_ready()` indicates that the network stack is initialized; it does **not** by itself confirm an obtained IP. Read `wifi` and `ip` in device status before opening the portal.

The camera, recording and playback are **not kept continuously running by `app_main()`**. They are started as their commands or web routes need them.

## 3. A command from the user to the hardware

```text
USB/Serial command ─┐
                    ├→ jr_commands → registered Runtime API / module → HW04
HTTP POST /cmd ─────┘
```

The command layer handles legacy/local commands (`version`, `status`, expression names, audio diagnostics and Serial Wi-Fi setup). Requests prefixed with `api ` are passed to `jr_runtime_api_handle()`, which only permits registered functions such as `capabilities`, `get` and `face`.

**HTTP is more restricted than Serial.** Wi-Fi setup and certain diagnostics are deliberately Serial-only; a request header is a command marker, **not proof of user authentication**.

## 4. Portal, camera and real-time communication

| Path | Implementation |
| --- | --- |
| Local HTTP | Port **80**, root interface, status/commands, snapshot and recording routes |
| Local HTTPS | Port **443**, only when local certificate/key material is present |
| Camera | OV5640 → JPEG in `jr_camera_diag.c`; snapshots by `/capture` |
| Live video | JPEG frames → WebRTC **DataChannel** → browser |
| Live microphone | MS3625 → I2S RX → **PCMA/G.711 A-law** → WebRTC audio |
| Live speaker | Browser WebRTC audio → PCMA decode → I2S TX → MAX98357A |

The WebRTC signaling/status handlers are registered **only on HTTPS** by `jr_webrtc_audio_register_routes()`. When HTTPS material is absent the firmware logs `JR_HTTPS_DISABLED` and **WebRTC is unavailable in that build configuration**. The intended communication is local-network Live, not an Internet-facing service.

See [Development](DEVELOPMENT.md) for the missing tracked HTTPS provisioning script and [API](API_RUNTIME.md) for HTTP endpoints.

## 5. Modes, audio resource ownership and recovery

| Manager | Actual role |
| --- | --- |
| `jr_mode_manager` | Tracks `IDLE`, `AUTONOMOUS`, `LIVE`, `RECORDING`, `PLAYBACK`, `DIAGNOSTIC` |
| `jr_resource_manager` | Grants exclusive ownership of shared I2S resources to `LIVE`, `MIC`, `PLAYBACK` or `LEGACY_VOICE` |

In Live startup, `jr_webrtc_audio.c` saves the earlier mode, suspends autonomy as necessary, enters `LIVE` and acquires the I2S resource. On stop/failure it releases resources and attempts to restore the previous mode/voice state.

This addresses **resource conflicts**, but is not acoustic echo cancellation. A mode listed in the code is **not necessarily a fully validated product feature**. The previously documented HW04 Live validation pertains to the V1.7.04 baseline; consolidated V1.8S validation is a separate release check.

## 6. JrSkill Network execution — V1S / V1.8S

```text
Solana Devnet — Skill data + License PDA
                 ↓
Browser-side JrSkill App (embedded jrskill_app.js)
  - connect/authenticate wallet
  - confirm Devnet and licensed account
  - validate Skill JSON/hash and allowed actions
                 ↓
POST /cmd with api {"v":1,"fn":"face",...}
                 ↓
Firmware Runtime API → OLED module → visible expression
```

**Where code actually runs:** the JavaScript App is compiled **into the ESP32 firmware as a served browser asset**, but wallet/Devnet access, `wait` timers, recipes, licensing checks and Skill sequence execution run **in the browser**. The ESP32 executes approved firmware capabilities, not arbitrary uploaded scripts.

The current App recognizes **`face`**, bounded **`wait`**, and the installed **`face_sequence` recipe**. It verifies license ownership from Devnet and checks the device's `capabilities` before issuing physical actions. Store purchases happen outside this local App. **Solana Mainnet deployment is not implied**.

The planned general-purpose [Flow Engine](FLOWS.md) and [Playground](PLAYGROUND.md) are **separate future designs**; the existing JrSkill recipe runner must not be confused with a completed on-device Flow Engine.

## 7. Boundaries

- Hardware GPIO assignments are centralized in [HW04 Wiring](../02-assembly/WIRING.md), not duplicated here.
- API functions and inputs must be allowlisted and validated.
- Camera/audio are subject to PSRAM, I2S ownership, network and certificate availability.
- No production-grade remote authentication is established by the local command header.
- The `v2` offline-voice workstream remains separate from the finished V1S workstream.

Use [Testing](TESTING.md) to distinguish code present, software tests and observed physical behavior.
