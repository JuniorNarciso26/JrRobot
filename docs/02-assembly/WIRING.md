# JrBot — HW04 Wiring Reference

This is the **confirmed electronic pin map** for `JRBOT-HW-04`. Keep it synchronized with [`hardware/pinmap.json`](../../hardware/pinmap.json) and the [firmware's generated pin header](../../firmware/core/jr_pinmap_fixed.h).

## Quick connection map

| Component | Module signal | ESP32-S3 / supply |
| --- | --- | --- |
| OLED SSD1306 | SDA / SCL | **GPIO1 / GPIO2** |
| OLED SSD1306 | VCC / GND | **3V3 / GND** |
| Microphone MS3625 | SCK / WS / SD | **GPIO21 / GPIO47 / GPIO41** |
| Microphone MS3625 | VDD / GND / L/R | **3V3 / GND / GND** |
| Amplifier MAX98357A | BCLK / LRC / DIN | **GPIO21 / GPIO47 / GPIO42** |
| Amplifier MAX98357A | VIN / GND | **5V / GND** |
| Speaker 4 Ω / 3 W | Positive / negative | **MAX98357A OUT+ / OUT-** |

**Important:** the microphone and amplifier intentionally **share the same I2S BCLK (GPIO21) and WS (GPIO47)**. The microphone supplies RX data on GPIO41; amplifier output data uses GPIO42.

## MAX98357A — seven-pin header, as documented for HW04

| Header order | Pin | Connection |
| ---: | --- | --- |
| 1 | `LRC` | GPIO47 |
| 2 | `BCLK` | GPIO21 |
| 3 | `DIN` | GPIO42 |
| 4 | `GAIN` | Leave unconnected in the HW04 reference assembly |
| 5 | `SD` | Leave unconnected in the HW04 reference assembly |
| 6 | `GND` | GND |
| 7 | `VIN` | 5V |

Connect the speaker **between `OUT+` and `OUT-` only**. Do **not** connect either speaker lead to ground. Confirm the exact module's printed pin labels before applying power.

## OV5640 camera connection

The OV5640 connects to the controller board's **dedicated camera connector**; it is not intended to be rewired as individual jumper cables during normal HW04 assembly.

| Camera signal | GPIO |
| --- | ---: |
| XCLK | 15 |
| SCCB SDA / SCL | 4 / 5 |
| D0 / D1 / D2 / D3 | 11 / 9 / 8 / 10 |
| D4 / D5 / D6 / D7 | 12 / 18 / 17 / 16 |
| VSYNC / HREF / PCLK | 6 / 7 / 13 |
| PWDN / RESET | -1 / -1 (not assigned in this board map) |

## USB and port identification

The development reference reported **COM6 for flashing** and **COM4 for panel/Serial commands**. These are **historical Windows port assignments**. Discover the actual COM ports on each computer and select them in the installer or panel; the numbers are not fixed.

## Safety and constraints

- Disconnect USB/power before connecting or moving wires. Inspect supply voltage, common ground, orientation and shorts before powering on.
- Do not power motors or servos directly from the ESP32-S3's 3V3 rail.
- Camera, flash/PSRAM, USB and boot pins already have reserved functions. Do not reassign them without a pin-policy review.
- GPIO35–37 are used internally for octal PSRAM on N16R8. GPIO39/40 belong to the JTAG/debug group and are **not approved motor-control choices in the current plan**.
- **No motor wiring is part of the confirmed HW04 build.** Proposed movement pins belong in the [Roadmap](../01-history-roadmap/ROADMAP.md), not in this assembly reference.

See the [HW04 Hardware Reference](HARDWARE_HW04.md) for module notes and [Assembly Guide](ASSEMBLY_GUIDE.md) for the order of work.
