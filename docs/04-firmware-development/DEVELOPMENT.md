# JrBot — Development, Installation and First Run

This is the operational guide for the firmware currently recorded on **`V1s-00`**. For the internal startup sequence use [Architecture](ARCHITECTURE.md); for commands use [Runtime API](API_RUNTIME.md).

## 1. Before you begin

| Item | Requirement |
| --- | --- |
| Controller | ESP32-S3 N16R8 and the documented HW04 electronics |
| Firmware toolchain | **ESP-IDF 5.5.x** on Windows; the component manifest requires `>=5.5.0,<5.6.0` |
| Repository | Git, a clean working tree and the intended branch |
| Panel | Python 3 (Windows `py -3` launcher) |
| Connections | USB **data** cable; the computer selects the detected serial port |

See [Hardware Assembly](../02-assembly/README.md) for components and [Wiring](../02-assembly/WIRING.md) for connections. **Do not use old fixed COM port numbers.**

Keep the panel Python environment separate from ESP-IDF: `PAINEL.bat` uses `%LOCALAPPDATA%\JrBot\panel-venv`. Do not install panel requirements in Espressif's private Python environment. An October 2026 incident with incompatible `cryptography` versions was fixed by this separation.

## 2. Select the code to install

| Branch | Purpose |
| --- | --- |
| `main` | Latest promoted stable baseline: **JrBot_V1.7.04** |
| `V1s-00` | **V1.8S development completed**, undergoing documentation and release checks |
| `develop` | Integration baseline |
| `v2` | Separate experimental local-voice work |
| `v1` | Historical reference |

The Windows installer **consults available branches and synchronizes the selected repository**, including when invoked with an action such as `build` or `flash`. Review your work before running it: it rejects ordinary local modifications rather than overwriting them.

To prepare the integration branch manually:

```bash
git switch V1s-00
git pull --ff-only origin V1s-00
```

## 3. Build, flash and start the development panel

From the **repository root** on Windows:

```bat
INSTALAR.bat build
INSTALAR.bat flash
PAINEL.bat
```

- `build`: selects/synchronizes the branch, loads ESP-IDF, checks the pin map and compiles the firmware.
- `flash`: performs the build checks, asks the computer to identify/select the serial port, then flashes the ESP32-S3.
- `PAINEL.bat`: starts the **computer-side** panel at **`http://127.0.0.1:8765`**. It is **not** the web portal hosted by the robot.
- `INSTALAR.bat` (without an argument) runs the installer's default `all` flow; `INSTALAR.bat help` lists the supported actions.

The installer runs `tools/generate_pinmap.py --check` and `tests/hardware/test_pin_policy.py` before building. To run those checks independently:

```bash
python tools/generate_pinmap.py --check
python tests/hardware/test_pin_policy.py
```

The project targets `esp32s3`; CMake uses `firmware/version.txt`, the HW04 configuration and the ESP-IDF component manifest. **A successful compile or flash is not a physical validation.**

**Port-selection prerequisite:** `tools/select_port.py` relies on `pyserial` in the Python environment used by the installer. If no ports appear or the module is missing, resolve that environment issue and check the USB data cable before retrying.

## 4. Configure Wi-Fi on the robot

The ESP32-S3 runs as a **Wi-Fi station**, not a guaranteed standalone access point. A fresh board without valid saved credentials cannot be expected to serve its browser interface.

**Recommended procedure from the existing panel:**

1. Connect the board by USB and open `PAINEL.bat`.
2. Select the **USB/Serial** connection and the device port found by the computer.
3. Enter the SSID, password, hostname and **DHCP** (default) or a validated static IPv4 configuration in the panel's Wi-Fi form.
4. Save. The panel sends the firmware's **`wifi_config_pct`** command through Serial.
5. Confirm `JR_WIFI_SALVO pending_restart=1`, then **restart the ESP32-S3**.
6. Use the device's `status` response to confirm network connection and the assigned IP. Access the robot's own portal at `http://<device-ip>/` on the local network.

The firmware stores a versioned Wi-Fi record in **NVS**. The Serial-only `wifi_clear` command removes its saved connection and likewise requires a restart.

An alternative configuration source is `firmware/main/wifi_config.local.h`, copied locally from [the example](../../firmware/main/wifi_config.local.h.example) **before compiling**. Never commit SSIDs, passwords or private keys. The example refers to `CONFIGURAR_WIFI.bat`, but **that script is not tracked in this branch**; do not assume it exists.

## 5. Understand HTTPS and Live availability

The firmware serves the ordinary robot portal through **HTTP port 80** when its network stack is initialized. **HTTPS port 443 and the WebRTC routes are conditional.**

In `firmware/module_portal/jr_portal.c`, HTTPS is compiled/started only when the local, ignored header `firmware/main/jr_https_material_local.h` is available and enables `JR_HTTPS_LOCAL_MATERIAL_AVAILABLE`. The implementation reads `JR_HTTPS_CERT_PEM` and `JR_HTTPS_KEY_PEM` from that header.

- If this local material is absent, the log reports **`JR_HTTPS_DISABLED`** and **WebRTC cannot be assumed available**, even though the firmware builds.
- If properly configured and started, look for **`JR_HTTPS_READY`** and the `/webrtc-audio` route on HTTPS.
- The log mentions `CONFIGURAR_HTTPS_V1`, but **no corresponding configuration script is tracked in `V1s-00`**. The local certificate/key provisioning procedure is therefore an **open reproducibility gap**. Do not invent a setup command or commit secrets to the repository.
- Browsers and wallet APIs may require a secure context and appropriate trust of the local HTTPS certificate.

This section documents the **implemented condition**, not a claim that a clean checkout includes working HTTPS credentials. See [Architecture](ARCHITECTURE.md) for the resulting Live data paths.

## 6. Confirm startup and exercise the hardware

At boot, `main.c` prints the build identifier and HW04 pin profile, initializes commands/terminals and the OLED, then starts Wi-Fi and the portal where possible. Camera and audio functions are generally initialized **on demand**.

From Serial or the panel, start with:

```text
version
status
help
```

Then verify each actual result using [Testing](TESTING.md):

| Check | What to observe |
| --- | --- |
| `happy` (or a listed face command) | OLED visibly changes |
| `camera_test` / local `/capture` | JPEG capture and viewable image |
| `mic_test`, `/mic-record` | Microphone response and audible recording |
| `audio_test` | Speaker produces a sound — log success alone is insufficient |
| `/webrtc-audio` over HTTPS | Both audio directions and video on the same local network |
| JrSkill App on device | Authenticated Devnet wallet, licensed Skill and physically observed allowed action |

The firmware reports **`JrBot_V1S_APP_03`** in `firmware/version.txt` and `core/jr_config.h`, although the release milestone is named **V1.8S**. Treat the filename/build identifier as the real installed firmware identity until release metadata is deliberately aligned.

## 7. Common diagnostic cases

| Symptom | First place to check |
| --- | --- |
| ESP-IDF cannot start | ESP-IDF 5.5.x and its private Python dependencies; do not reuse panel venv |
| Flash port absent | USB data cable, drivers and selected port detected by the computer |
| No device web page | Saved Wi-Fi settings, reboot after saving, Serial `status` and assigned IP |
| HTTP works but Live does not | `JR_HTTPS_DISABLED` vs `JR_HTTPS_READY`; certificate provisioning |
| OLED offline | Physical wiring, initial `jr_face_start`, retry logs |
| Camera test fails | HW04/OV5640 connector, PSRAM availability, `jr_camera_diag.c` |
| Audio is busy | Mode/resource ownership diagnostics; stop other I2S users |
| Wallet cannot run a Skill | Secure browser context, Devnet/auth, current license, Skill schema and `face` capability |

## 8. Contributing and release discipline

Use **Issue → implementation → build → tests → documentation → review → approved promotion**. Do not change or promote `main` automatically.

Keep active documents in English: [Project Status](../01-history-roadmap/PROJECT_STATUS.md) = current work; [Changelog](../01-history-roadmap/CHANGELOG.md) = history; [Roadmap](../01-history-roadmap/ROADMAP.md) = future plans. Each PR must identify affected modules, tested branch/build, physical evidence (or pending status), known limitations and updated documentation.
