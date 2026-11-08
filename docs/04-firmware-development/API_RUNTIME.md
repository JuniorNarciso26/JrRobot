# JrBot — Commands, HTTP Interfaces and Runtime API

This document describes **three distinct interfaces** implemented by the current firmware: legacy Serial commands, local HTTP/HTTPS routes and the **Runtime API v1**. They are not interchangeable.

## 1. Serial/USB command interface

Commands are plain text passed to `jr_commands.c` through UART/USB terminal paths.

| Command | Function |
| --- | --- |
| `help`, `version`, `status` | Discover commands, installed build and module/network state |
| `happy`, `neutral`, `surprised` and other listed expressions | Change the OLED face |
| `camera_test` | Perform the local camera diagnostic (**Serial-only**) |
| `mic_test`, `mic_status` | Microphone diagnostics |
| `audio_test`, `audio_diag`, `audio_play_recording` | Audio diagnostics and last-recording playback |
| `audio_volume 50` | Set output volume from 0 to 100 |
| `autonomo_on`, `autonomo_off`, `brain_status`, `brain_test` | Experimental local-brain control and simulation/diagnostics |
| `wifi_config_pct ...`, `wifi_clear` | Save or clear Wi-Fi settings via **Serial only**, then restart |
| `api {JSON}` | Invoke a registered Runtime API function |

For Wi-Fi, the **existing development panel** assembles and sends `wifi_config_pct` safely, with encoded SSID/password and optional static IP fields. Use its form instead of manually typing credentials into shared logs. See [Development](DEVELOPMENT.md).

**Important:** `brain_test` exercises the test path; it does not prove that a real spoken wake word was recognized.

## 2. Firmware HTTP routes

The device starts **HTTP on port 80** after network initialization. Additional routes are registered on HTTPS when local certificate material is provisioned.

| Method and path | Behavior |
| --- | --- |
| `GET /` | Embedded browser interface, including Live/JrSkill browser assets |
| `GET /status` | Device status text |
| `POST /cmd` | Command text with `X-JrBot-Command: 1` |
| `GET /cmd?c=status` or `?c=help` | Legacy read-only command access (only these two values) |
| `GET /capture` | JPEG camera snapshot |
| `GET /mic-record` | Record microphone audio and return WAV data |
| `GET /mic-live`, `/mic-stream`, `/mic-ws` | Legacy microphone live/stream/WebSocket interfaces |
| `POST /audio` | Receive supported WAV/audio input |
| `GET /live-diag` | Legacy Live microphone diagnostics |
| `GET /jrskill-app.js` | Browser-side JrSkill application asset |
| `GET /memory-panel.js`, `GET /wifi-panel.js` | Browser scripts for RAM diagnostics and connected-mode Wi-Fi tab |
| `GET /autofocus`, `/manual-focus` | Currently return **503**, not an implemented focusing control |

**HTTPS-only WebRTC endpoints**, registered if HTTPS starts successfully:

| Method and path | Behavior |
| --- | --- |
| `GET /webrtc-audio` | Local Live interface |
| `GET/POST /webrtc-audio/signal` | Session signaling |
| `GET /webrtc-audio/status` | Live session status |
| `GET /webrtc-audio/lab` | V1.8.2S Mesa Mix settings and audio/RAM indicators |
| `POST /webrtc-audio/lab` | Update temporary Live audio settings during a session |

**V1.8.3S Wi-Fi management API (HTTPS-only)** — these routes are separate from the Serial command interface and are registered only when the HTTPS service starts:

| Method and path | Behavior |
| --- | --- |
| `GET /wifi-manager?view=status` | Current connection, SSID (percent-encoded), IP, RSSI, channel, Wi-Fi auth type, operating mode and pending restart; no saved password |
| `GET /wifi-manager?view=profiles` | Up to four saved SSID profiles, selected status and DHCP/static configuration indicator; no credentials returned |
| `GET /wifi-manager?view=scan` | On-demand scan of nearby networks, limited to eight displayed; available only in `IDLE` mode |
| `POST /wifi-manager` | JSON actions `save`, `use`, `delete` (profile id 0–3), or `restart`; modifying operations require `IDLE`, JSON content type and `X-JrBot-WiFi: 1` |

`save` requires `ssid` and `password` and stores a profile without disconnecting the current station. `use` selects a saved profile for the next boot, and `delete` erases that profile from NVS after the browser confirms. A selected profile can be deleted but requires a stronger warning because initial setup may be needed on the next boot. `restart` executes only when a Wi-Fi change is pending and triggers a deliberate device reboot. The UI is documented in [Wi-Fi Dashboard](WIFI_DASHBOARD.md).

**Security:** Wi-Fi writes are accessible through the local HTTPS listener with origin/header restrictions, **not login authentication**. The ordinary `POST /cmd` still blocks all `wifi_*` network commands. Neither the Wi-Fi dashboard nor its API should be published to the Internet.

The HTTPS requirement is **conditional on local `jr_https_material_local.h`**; a clean checkout does not contain this ignored certificate/key material. If the device logs `JR_HTTPS_DISABLED`, do not expect the WebRTC routes to exist. Refer to [Development](DEVELOPMENT.md).

**Security boundary:** The `X-JrBot-Command: 1` header is required on `POST /cmd`, but **is not authentication**. Firmware blocks Serial-style Wi-Fi configuration and `camera_test` from the `/cmd` network command path. The dedicated `/wifi-manager` endpoints above are a separate HTTPS-only interface, not a bypass through `/cmd`. Do not expose this interface directly to the public Internet.

## 3. Runtime API v1 contract

| Property | Value |
| --- | --- |
| Major protocol | `1` |
| Implementation | `1.1` |
| Functions | `capabilities`, `get`, `face` |
| Reply prefix | `JR_API ` |
| No general flow engine | `flow_engine=false` and `playground=false` |

### Example calls

Serial:

```text
api {"v":1,"fn":"capabilities","args":{}}
api {"v":1,"fn":"get","args":{"path":"system.version"}}
api {"v":1,"fn":"face","args":{"expression":"happy"}}
```

Via local HTTP:

```text
POST /cmd
X-JrBot-Command: 1
Content-Type: text/plain

api {"v":1,"id":"req01","fn":"face","args":{"expression":"happy"}}
```

Example success-envelope shape:

```text
JR_API {"v":1,"ok":true,"id":"req01","result":{...}}
```

On failure, the same envelope includes `ok=false` and an `error` object. `id` is optional.

The `get` function supports these registered paths:

```text
api.version        system.version      system.hardware
system.profile     brain.status        voice.status
audio.volume       face.current        wifi.status
wifi.diagnostics
```

The `face` expression must be recognized by the firmware's OLED renderer; arbitrary names or direct GPIO writes are not authorized.

### Errors

| Code | Meaning |
| --- | --- |
| `invalid_json` | Invalid JSON |
| `invalid_request` | Bad envelope, function name or identifier |
| `invalid_args` | Invalid arguments or expression |
| `unsupported_version` | Incorrect major protocol |
| `not_found` | Unknown function/path |
| `no_memory` | Allocation/serialization failure |
| `response_too_large` | Response exceeds the output buffer |

**Only registered Runtime API functions are callable**. This is not arbitrary Python/C, a shell, raw-memory access or motor control. New functions must be advertised through `capabilities`; incompatible changes require a protocol-major change.

## 4. JrSkill is a client of this API

The `jrskill_app.js` browser App checks a Devnet license and JSON payload, then sends registered `face` calls through `POST /cmd`. Browser-side `wait` and `recipe` steps do **not** imply new ESP32 Runtime API functions.

See [Architecture](ARCHITECTURE.md) for the complete action path and [JrSkill Network](../05-jrskill-network/README.md) for the blockchain accounts and licensing process.
