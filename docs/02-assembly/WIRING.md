# JrBot Wiring and Hardware Reference — HW04

**JRBOT-HW-04** uses an ESP32-S3 N16R8 (16 MB Flash, 8 MB PSRAM), OV5640 camera, SSD1306 OLED, MS3625 microphone and MAX98357A amplifier. This file is the **single human-readable reference** for HW04 electrical connections and constraints.

## Electrical connections

Connect **one module pin to the destination shown on each row**. The component names repeat intentionally so that each wire can be checked independently.

| Component | Pin on module | Connect to |
| --- | --- | --- |
| **SSD1306 OLED** | `SDA` | ESP32-S3 **GPIO1** |
| SSD1306 OLED | `SCL` | ESP32-S3 **GPIO2** |
| SSD1306 OLED | `VCC` | **3V3** |
| SSD1306 OLED | `GND` | **GND** |
| **MS3625 microphone** | `SCK` | ESP32-S3 **GPIO21** |
| MS3625 microphone | `WS` | ESP32-S3 **GPIO47** |
| MS3625 microphone | `SD` | ESP32-S3 **GPIO41** |
| MS3625 microphone | `VDD` | **3V3** |
| MS3625 microphone | `GND` | **GND** |
| MS3625 microphone | `L/R` | **GND** |
| **MAX98357A amplifier** | `BCLK` | ESP32-S3 **GPIO21** |
| MAX98357A amplifier | `LRC` | ESP32-S3 **GPIO47** |
| MAX98357A amplifier | `DIN` | ESP32-S3 **GPIO42** |
| MAX98357A amplifier | `VIN` | **5V** |
| MAX98357A amplifier | `GND` | **GND** |
| MAX98357A amplifier | `GAIN` | **Not connected** (HW04 reference) |
| MAX98357A amplifier | `SD` | **Not connected** (HW04 reference) |
| **4 Ω / 3 W speaker** | Speaker `+` | Amplifier **OUT+** |
| 4 Ω / 3 W speaker | Speaker `−` | Amplifier **OUT−** |

**Shared I2S signals:** the microphone and amplifier use the same BCLK (**GPIO21**) and WS (**GPIO47**). Microphone data enters through **GPIO41**; amplifier data leaves through **GPIO42**. The speaker must connect across the amplifier outputs; neither lead goes to GND.

The confirmed MAX98357A **seven-pin header order** is `LRC, BCLK, DIN, GAIN, SD, GND, VIN`. Always confirm labels on the specific breakout board before applying power.

## OV5640 camera

Attach the OV5640 using the controller board's **dedicated camera connector**, not individual jumper wires.

| Camera signal | ESP32-S3 GPIO |
| --- | ---: |
| `XCLK` | 15 |
| `SCCB SDA` | 4 |
| `SCCB SCL` | 5 |
| `D0` | 11 |
| `D1` | 9 |
| `D2` | 8 |
| `D3` | 10 |
| `D4` | 12 |
| `D5` | 18 |
| `D6` | 17 |
| `D7` | 16 |
| `VSYNC` | 6 |
| `HREF` | 7 |
| `PCLK` | 13 |
| `PWDN` | Not assigned (`-1`) |
| `RESET` | Not assigned (`-1`) |

The previously validated camera orientation is **−90° / 270°**.

## Constraints and validation

- **GPIO26–37:** reserved for Flash/PSRAM-related functions in the HW04 pin policy; GPIO35–37 are used by octal PSRAM.
- **GPIO39/40:** JTAG/debug functions. These are not approved for the proposed motor controls.
- **GPIO0/3/45/46**, native USB and camera pins: reserved or assigned to boot and board functions.
- The current firmware includes I2S microphone RX and OLED support. Earlier notes stating otherwise describe superseded diagnostic revisions.
- V1.7.04 validated camera, microphone, playback and local WebRTC on HW04 and introduced I2S resource arbitration. These records do not replace a consolidated V1.8S release test.
- Motors and servos require their **own driver and power design**. Proposed motion controls are tracked in the [Roadmap](../01-history-roadmap/ROADMAP.md), not defined by this wiring map.

**Sources of truth:** [`hardware/pinmap.json`](../../hardware/pinmap.json), [generated firmware definitions](../../firmware/core/jr_pinmap_fixed.h), [pin policy tests](../../tests/hardware/test_pin_policy.py), and the [original assembly confirmation](../../hardware/CONFIRMAR_MONTAGEM.txt).

For a practical build sequence, see the [Assembly Guide](ASSEMBLY_GUIDE.md).
