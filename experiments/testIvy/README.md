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
The original Ivy repository is tracked as a **pinned Git submodule** at `experiments/testIvy/ivy-upstream` (revision `35b2b77088a511a9fdaf3ee5f06ac7ed7bd069cc`). Clone it with:

```sh
git switch testIvy
git submodule update --init experiments/testIvy/ivy-upstream
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
The separate `audio_hw04` ESP-IDF project provides an initial no-codec I2S
RX/TX hardware smoke test. It is **not yet hooked up to Ivy STT/TTS/LLM**.

## Audio-only test (manual hardware validation)

Use the JrBot toolchain ESP-IDF 5.5.x for this independent minimal test.
**Recommended:** run `INSTALAR.bat` from the repository root and select the `testIvy` branch. On this branch, `build`, `flash`, and the default `all` action route to the **audio-only** project, not the normal JrBot firmware. Flashing asks for explicit confirmation. The `panel` action remains the original PC panel and is not used for this test.

```bat
INSTALAR.bat build
INSTALAR.bat flash
```

After flashing, open a serial monitor at the selected COM port to see the `MIC` logs. This does not yet run the original Ivy voice assistant.

```sh
cd experiments/testIvy/audio_hw04
idf.py set-target esp32s3
idf.py build
idf.py -p YOUR_SERIAL_PORT flash monitor
```

Expected behavior: **no automatic speaker tone**; Serial
`MIC frames=... peak=... avg_abs=...` lines which change when you speak.
Send a command with Enter via the ESP-IDF serial console (terminal must support sending input):

- `teste som` — temporarily pauses microphone I2S, emits a 1.5-second 440-Hz tone using 16-bit stereo Philips I2S (the same sample/slot format as the original JrBot `jr_audio_test_tone()`), then restores microphone capture. Look for `SPEAKER_TEST_START` / `SPEAKER_TEST_END result=ESP_OK` in the logs. `ESP_OK` confirms data transfer, **not audible sound**.
- `iniciar ivy` — currently replies `IVY_NAO_DISPONIVEL`, because Ivy STT/LLM/TTS and model storage are not integrated yet. **It does not activate Ivy**.
- `ajuda` — lists the commands.

This is an experimental serial interface and requires build/hardware validation. The sound test is run ONLY on command.
No Wi-Fi, SD, camera, OLED, or WebRTC task is started. Microphone channel is
assumed LEFT (L/R tied to GND), 32-bit I2S RX sampled as upper 16 bits.
**Build and physical validation have NOT been performed here.**

After this succeeds, replace the Ivy ES8311-specific board implementation with
a HW04-compatible adapter while keeping the upstream submodule unchanged;
then assess model storage (the original Ivy uses SD) and the ESP-IDF 6.1
requirement. No claim of complete offline Ivy operation yet.


## Next stage after speaker and microphone hardware confirmation (2026-11-22)

The user physically confirmed that the `teste som` Serial command produces sound.
The microphone I2S log also showed varying samples. Audio hardware bring-up can be
considered provisionally validated. Full audio recording quality, STT and TTS are
not yet validated.

**Do not label `iniciar ivy` as operational yet.** In the current program it
correctly returns `IVY_NAO_DISPONIVEL`.

### Verified upstream dependency

Ivy `components/story_stt/include/story_stt.h` documents
`stt_open(model_path, fast_arena, bulk_arena)`, `stt_push_chunk` (5760
samples of 16 kHz signed PCM) and `stt_transcribe`.
Its memory budget `docs/MEMORY_BUDGET.md` lists a **13.8 MB conformer STT
model.bin streamed from microSD**, with approximately 5.4-6.97 MB PSRAM
working memory and a 256 KB contiguous internal RAM workspace. This cannot
be assumed to fit an ESP32-S3 N16R8 16 MB flash image alongside the app.
Ivy upstream targets ESP-IDF 6.1 while the independent audio smoke test
targets the JrBot ESP-IDF 5.5.x toolchain.

### Next implementation milestone

1. Confirm external model storage. Ivy upstream's SDMMC pins cannot be used
   unchanged on HW04: GPIO41 is microphone DIN and GPIO47 is the I2S WS;
   other camera pins are also allocated. Plan an independent SPI microSD
   interface only after a free-pin / camera-conflict review.
2. Create an Ivy-compatible HW04 board audio adapter (16 kHz signed PCM
   input, output to MAX98357A) around the validated I2S functions.
3. Provide an isolated application with `teste som` and `iniciar ivy`:
   `iniciar ivy` must perform actual SD/model initialization, report
   explicit failures, then accept and transcribe voice. Do not report success
   if the STT weights are missing.
4. Evaluate further LLM/TTS storage and memory requirements separately;
   do not enable camera, OLED, Wi-Fi or WebRTC during initial Ivy runtime.
5. Validate flash/build logs and physical voice transcription before merging
   any integration work. Keep `main` untouched.

See upstream files:
- `https://github.com/iamankushpandit/esp_ai/blob/main/docs/MEMORY_BUDGET.md`
- `https://github.com/iamankushpandit/esp_ai/blob/main/components/story_stt/include/story_stt.h`
