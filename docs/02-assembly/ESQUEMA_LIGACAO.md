# JrBot — HW04 Final Wiring Diagram

## MAX98357A — seven-pin connector

| Order | Pin | Connect to |
| ---: | --- | --- |
| 1 | LRC | GPIO47 |
| 2 | BCLK | GPIO21 |
| 3 | DIN | GPIO42 |
| 4 | GAIN | **DO NOT CONNECT** |
| 5 | SD | **DO NOT CONNECT** |
| 6 | GND | GND |
| 7 | VIN | 5V |

Connect the 4-ohm / 3 W speaker **only between `OUT+` and `OUT-`** on the MAX98357A. Neither speaker terminal connects to GND.

## MS3625 I2S microphone

| MS3625 | ESP32-S3 |
| --- | --- |
| SCK | GPIO21 |
| WS | GPIO47 |
| SD | GPIO41 |
| VDD | 3V3 |
| GND | GND |
| L/R | GND |

## SSD1306 128 × 64 OLED

| OLED | ESP32-S3 |
| --- | --- |
| SDA | GPIO1 |
| SCL | GPIO2 |
| VCC | 3V3 |
| GND | GND |

If the OLED is disconnected, JrBot continues running and attempts to detect the display again.

## OV5640 camera

The camera remains attached to the board's original camera connector. The **Refresh status** function checks for a sensor response and enables the test when the camera is detected.

## Control panel behavior

- **Camera:** enables testing only after detection.
- **Microphone:** enables testing only when valid I2S activity is observed.
- **OLED:** enables facial-expression testing after the display responds.
- **MAX98357A:** its test remains available because this amplifier has no feedback/ACK signal for physical-presence detection.

## USB

- **COM4:** panel/commands in the reference setup.
- **COM6:** flashing in the reference setup.
