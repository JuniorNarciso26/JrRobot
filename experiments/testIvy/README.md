# testIvy — isolated Ivy AI evaluation on JrBot HW04

Status: **source integration preparation, not flashed or validated**.

Upstream: https://github.com/iamankushpandit/esp_ai (GPL-3.0-or-later).
Pinned upstream revision: `35b2b77088a511a9fdaf3ee5f06ac7ed7bd069cc`.

## Goals
1. Preserve JrBot `main` firmware and all current features.
2. Evaluate the **unmodified Ivy AI upstream** independently.
3. Adapt only the HW04 microphone (MS3625 I2S) and speaker (MAX98357A I2S).
4. Disable Wi-Fi, camera, WebRTC, OLED/graphical UI and other JrBot features in the *experimental target*, not in the existing firmware.
5. Verify audio loopback and model dependencies before attempting STT/LLM/TTS.

## Obtain upstream source
The original Ivy repository is **not vendored as a complete copy** in this branch.
The `.gitmodules` file records the intended upstream reference, but until a
Git gitlink is committed it is not a fully registered submodule. Clone explicitly:

```sh
git clone https://github.com/iamankushpandit/esp_ai.git ivy-upstream
cd ivy-upstream
git checkout 35b2b77088a511a9fdaf3ee5f06ac7ed7bd069cc
```

Keep the upstream source unchanged and develop modifications as an overlay.

## Pin mapping

| Signal | Ivy FNK0104B | JrBot HW04 |
|---|---:|---:|
| I2S BCLK | 5 | 21 |
| I2S WS | 7 | 47 |
| I2S microphone DIN | 6 | 41 |
| I2S speaker DOUT | 8 | 42 |
| I2S MCLK | 4 | unused |
| Analog codec | ES8311 (I2C) | none |
| Speaker amp control | GPIO1 | MAX98357A no dedicated control |

Use 16 kHz, 16-bit, mono for Ivy. Verify MS3625 sample layout/slot before STT.
JrBot HW04 already uses GPIO21 and GPIO47 as a shared clock pair.
See `docs/02-assembly/WIRING.md`.

## Important blockers before flashing
- Original Ivy is designed for the FNK0104B (ES8311, LCD/touch, SDMMC, battery).
  A simple GPIO edit **will not** run the complete application on JrBot.
- Ivy loads STT/TTS/LLM assets from microSD. JrBot HW04 has no documented
  usable microSD interface; investigate custom flash partitions after audio.
- Original Ivy requires ESP-IDF v6.1 while current JrBot development uses
  a different version. Use a separate build environment.
- GPL-3.0-or-later applies to Ivy's original code; keep copyright/notice files.
- No verified hardware testing, build, STT, TTS or inference on JrBot yet.

## Scope of this branch
The upstream reference and HW04-specific settings are recorded in
`hw04_audio_config.h`. This branch does **not** enable or alter the JrBot
production audio driver or attempt to boot Ivy's incompatible board startup.
Next implementation task: implement an alternate Ivy `board_audio.c` for
HW04 with a no-codec I2S RX/TX path, then introduce an audio-only entrypoint.
