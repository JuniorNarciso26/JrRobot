# JrBot — Hardware Reference JRBOT-HW-04

## Reference revision

**JRBOT-HW-04** is the documented electronics baseline used by the current JrBot development line.

| Module | Reference |
| --- | --- |
| Controller | ESP32-S3 N16R8, dual USB-C |
| Camera | OV5640 |
| Display | SSD1306 I2C OLED, 128 × 64 |
| Microphone | MS3625 I2S MEMS |
| Amplifier | MAX98357A mono I2S |
| Speaker | 4 Ω / 3 W |

The controller includes **16 MB Flash and 8 MB PSRAM**. The camera uses its dedicated board connector.

## Hardware validation notes

- **Shared audio clocks:** BCLK GPIO21 and WS GPIO47 are used by both microphone and amplifier; microphone SD is GPIO41 and amplifier DIN is GPIO42.
- **OLED:** I2C on GPIO1/GPIO2; firmware contains the OLED renderer and availability/recovery handling.
- **Microphone:** the current code implements an I2S RX driver; notes in older HW04 diagnostic records saying the driver was missing are obsolete.
- **OV5640:** the camera orientation physically confirmed for the later V1.6.3/V1.7.04 baseline was **-90° / 270°**.
- **Resource coordination:** V1.7.04 consolidated I2S ownership for mic, playback and Live. Hardware components were tested as part of that local-control baseline.

## Electrical allocation constraints

- **GPIO26–37** include Flash/PSRAM-related reservations; on N16R8, GPIO35–37 are associated with internal octal PSRAM.
- **GPIO39/40** are JTAG/debug-related pins, not Flash/PSRAM pins. They are intentionally avoided for motion-control plans following earlier observations.
- **GPIO0/3/45/46**, native USB pins and camera pins have other board/boot roles documented by the pin policy.
- **Motion is not implemented in the current HW04 firmware.** Planning for its future control signals is kept in the [Roadmap](../01-history-roadmap/ROADMAP.md).

## Authoritative technical references

- [Wiring Reference](WIRING.md) — pin tables and connection rules.
- [Machine-readable pin map](../../hardware/pinmap.json) — source for generated pin definitions.
- [Generated firmware pin header](../../firmware/core/jr_pinmap_fixed.h).
- [Original assembly confirmation](../../hardware/CONFIRMAR_MONTAGEM.txt).
- [Hardware pin policy tests](../../tests/hardware/test_pin_policy.py).

The older HW03 records remain in `trash/` for historical review. A new V1.8S consolidated physical validation must be recorded separately; do not infer it from previous tests.
