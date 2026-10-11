# JrBot — HW04 Hardware Review

**JRBOT-HW-04** is the current reference hardware revision.

## Confirmed components

- ESP32-S3 N16R8.
- OV5640 camera.
- SSD1306 OLED display.
- MS3625 I2S microphone.
- MAX98357A amplifier.

## I2S audio

Physically used pin mapping:

```text
BCLK / SCK     GPIO21
WS / LRCLK     GPIO47
MS3625 SD      GPIO41
MAX98357A DIN  GPIO42
```

BCLK and WS are intentionally shared between the microphone (RX) and amplifier (TX).

The stable **`JrBot_V1.7.04`** baseline supports simultaneous I2S RX/TX for local WebRTC Live and centralizes I2S resource ownership.

## Camera

**OV5640.** Physically confirmed camera orientation:

```text
-90° / 270°
```

Preserve this orientation in normal firmware builds, except during a dedicated camera experiment.

## OLED

**SSD1306**, connected by I2C through GPIO1/GPIO2 in the official pin map.

## Sources of truth

Review these references together before changing hardware:

- `hardware/pinmap.json`.
- [HW04 Pin Mapping](PINAGEM.md).
- [Wiring Diagram](ESQUEMA_LIGACAO.md).
- GPIO validation tests and policies.

Older HW03 documentation and diagnostic profiles were moved to `trash/` for historical review.
