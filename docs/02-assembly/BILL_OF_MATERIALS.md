# JrBot — Electronic Bill of Materials (HW04)

Components recorded for the **JRBOT-HW-04** reference build:

| Qty. | Component | Reference specification |
| ---: | --- | --- |
| 1 | Controller board | ESP32-S3 N16R8, dual USB-C, camera connector |
| 1 | Camera | OV5640, compatible with the board connector |
| 1 | OLED display | SSD1306, I2C, 0.96-inch, 128 × 64, four pins |
| 1 | Microphone | MS3625, I2S MEMS module |
| 1 | Audio amplifier | MAX98357A, mono I2S, Class D |
| 1 | Speaker | 4 Ω, 3 W (the earlier parts record describes a compact approximately 20 mm / 1-inch format) |
| Up to 2 | USB **data** cables | Flashing and panel/Serial connections in the documented reference setup |
| As needed | Jumper wires and connectors | Connections matching the HW04 wiring reference |

## Before buying parts

- Confirm that the ESP32-S3 board is the **N16R8 model with the expected OV5640 connector**, rather than assuming every ESP32-S3 board shares the same breakout.
- The display, microphone and amplifier need the **correct pin labels and power connections**; check module markings before wiring.
- USB port names such as `COM4` and `COM6` are *examples from the development PC*, not requirements for another computer.
- Motors, battery, charging circuit, body fasteners and 3D-printed parts are **not part of this confirmed electronic bill of materials**. They must be specified and validated separately.

**Next:** [Wiring Reference](WIRING.md) → [Assembly Guide](ASSEMBLY_GUIDE.md).
