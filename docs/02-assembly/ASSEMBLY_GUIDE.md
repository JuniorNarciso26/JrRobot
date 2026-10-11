# JrBot — Electronic Assembly Guide (HW04)

This guide organizes the **existing verified HW04 electronic information** into a practical assembly sequence. It does not describe unverified battery, motor, or mechanical-body assembly.

## 1. Identify the parts

Use the [Bill of Materials](BILL_OF_MATERIALS.md). Confirm you have an **ESP32-S3 N16R8** controller with an **OV5640** camera connector, an **SSD1306 OLED**, **MS3625 microphone**, **MAX98357A amplifier**, compatible speaker, and data-capable USB cable(s).

Identify each module's own **pin labels** before connecting it. Physical module layouts vary.

## 2. Connect the camera (power disconnected)

Use the board's original OV5640 camera connector. Check that the camera ribbon/connector is properly seated and oriented for **your board variant**; the documented pin map is not a substitute for the physical connector markings.

## 3. Connect the OLED (power disconnected)

| OLED | ESP32-S3 |
| --- | --- |
| SDA | GPIO1 |
| SCL | GPIO2 |
| VCC | 3V3 |
| GND | GND |

The firmware supports OLED detection and recovery if the display is temporarily unavailable.

## 4. Connect the I2S microphone (power disconnected)

| MS3625 | ESP32-S3 |
| --- | --- |
| SCK | GPIO21 |
| WS | GPIO47 |
| SD | GPIO41 |
| VDD | 3V3 |
| GND | GND |
| L/R | GND |

## 5. Connect the amplifier and speaker (power disconnected)

| MAX98357A | Connection |
| --- | --- |
| LRC | GPIO47 |
| BCLK | GPIO21 |
| DIN | GPIO42 |
| VIN | 5V |
| GND | GND |
| GAIN / SD | Unconnected in the HW04 reference assembly |

Connect the **4 Ω / 3 W speaker only to amplifier OUT+ and OUT-**, not to GND. The microphone and amplifier share BCLK and WS intentionally.

The complete mapping and camera GPIO reservations are in [Wiring Reference](WIRING.md).

## 6. Check before applying power

With the board disconnected from USB, check:

- Supply pins and GND correspond to the module labels and the HW04 reference.
- No exposed wire or unintended connection creates a short.
- Camera, OLED, microphone and amplifier connections match [Wiring Reference](WIRING.md).
- The speaker is connected across OUT+/OUT- only.
- No motor, servo or battery circuit has been added based on unvalidated assumptions.

Only energize the circuit once those checks have passed.

## 7. Install firmware and check functions

Follow the [Firmware Development Guide](../04-firmware-development/DEVELOPMENT.md) and the project's installer. Identify the real USB/COM ports in your system: **COM4/COM6 were examples**, not mandatory values.

After flashing a compatible build, check and record the firmware identifier, then perform these **separate observed tests**:

1. **OLED:** an expression is visibly displayed.
2. **Camera:** the OV5640 produces a visible image or JPEG preview.
3. **Microphone:** an actual audio recording contains audible input.
4. **Speaker:** the recorded or test audio is audibly reproduced.
5. **Local Live:** when supported by the installed firmware, confirm video and both audio directions on the local network.

A positive status message or successful compilation alone does not establish physical validation. Record the observed result for each component following the [Testing Guide](../04-firmware-development/TESTING.md).

## 8. Add the 3D-printed body later

Electronic assembly and body construction are separate tasks. Consult [3D Printing and Body](../03-3d-printing/README.md) once printable parts and assembly instructions have been collected and validated.

**Reference status:** the V1.7.04 HW04 baseline was physically tested for the documented camera, OLED/audio and local Live functions. This new guide does not imply that the entire consolidated **V1.8S** build has been retested.
