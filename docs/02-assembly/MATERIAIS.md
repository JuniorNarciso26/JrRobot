# JrBot — Confirmed Bill of Materials

| Quantity | Component | Identification |
| ---: | --- | --- |
| 1 | Main controller board | ESP32-S3-N16R8, two USB-C connectors, camera connector |
| 1 | Camera | OV5640 |
| 1 | Display | SSD1306 I2C OLED, 0.96 inch, 128 × 64, four pins |
| 1 | Microphone | MS3625 MEMS, I2S interface |
| 1 | Amplifier | MAX98357A, mono I2S, Class D |
| 1 | Speaker | 20 mm / 1 inch, 4 ohms, 3 W |
| 2 | USB data cables | One for flashing and one for the panel in this reference setup |
| As required | Wires and connectors | Signal and power connections |

## HW04 audio pin mapping

- **GPIO21:** shared BCLK/SCK.
- **GPIO47:** shared WS.
- **GPIO41:** microphone SD → ESP32.
- **GPIO42:** ESP32 → amplifier DIN.

**Historical note from the original materials record:** the microphone driver had not yet been implemented at the time that record was written, and amplifier testing was intended to remain on demand rather than start at boot. For validated microphone and Live capabilities, consult the [current Project Status](../01-history-roadmap/PROJECT_STATUS.md).
