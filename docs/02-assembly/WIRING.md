# JrBot Wiring and Hardware Reference — HW04

**JRBOT-HW-04** uses an ESP32-S3 N16R8 (16 MB Flash, 8 MB PSRAM), OV5640 camera, SSD1306 OLED, MS3625 microphone and MAX98357A amplifier. This file is the **single human-readable reference** for HW04 electrical connections and constraints.

## Electrical connections

| Device | Module pins | Connection |
| --- | --- | --- |
| **SSD1306 OLED** | SDA / SCL | GPIO1 / GPIO2 |
| | VCC / GND | 3V3 / GND |
| **MS3625 microphone** | SCK / WS / SD | GPIO21 / GPIO47 / GPIO41 |
| | VDD / GND / L/R | 3V3 / GND / GND |
| **MAX98357A amplifier** | BCLK / LRC / DIN | GPIO21 / GPIO47 / GPIO42 |
| | VIN / GND | 5V / GND |
| | GAIN / SD | Leave unconnected in the confirmed HW04 reference |
| **4 Ω / 3 W speaker** | + / − | MAX98357A OUT+ / OUT− (not GND) |

**Shared I2S signals:** the microphone and amplifier use the same BCLK (**GPIO21**) and WS (**GPIO47**). Microphone data enters through **GPIO41**; amplifier data leaves through **GPIO42**. The speaker must connect across the amplifier outputs; neither lead goes to GND.

The confirmed MAX98357A **seven-pin header order** is `LRC, BCLK, DIN, GAIN, SD, GND, VIN`. Always confirm labels on the specific breakout board before applying power.

## OV5640 camera

Attach the OV5640 using the controller board's **dedicated camera connector**, not individual jumper wires.

| Signal | GPIO | Signal | GPIO |
| --- | ---: | --- | ---: |
| XCLK | 15 | SCCB SDA / SCL | 4 / 5 |
| D0 / D1 / D2 / D3 | 11 / 9 / 8 / 10 | D4 / D5 / D6 / D7 | 12 / 18 / 17 / 16 |
| VSYNC / HREF / PCLK | 6 / 7 / 13 | PWDN / RESET | −1 / −1 (not assigned) |

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
