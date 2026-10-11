# JrBot Wiring and Hardware Reference — HW04

**JRBOT-HW-04** uses an ESP32-S3 N16R8 (16 MB Flash, 8 MB PSRAM), OV5640 camera, SSD1306 OLED, MS3625 microphone and MAX98357A amplifier. This file is the **single human-readable reference** for HW04 electrical connections and constraints.

## Electrical connections — grouped by accessory

Each row contains **all connections for one component**. Follow the pin names printed on your actual module.

| Accessory | Signal connections | Power and other connections |
| --- | --- | --- |
| **SSD1306 OLED** | `SDA` → **GPIO1**; `SCL` → **GPIO2** | `VCC` → **3V3**; `GND` → **GND** |
| **MS3625 microphone** | `SCK` → **GPIO21**; `WS` → **GPIO47**; `SD` → **GPIO41** | `VDD` → **3V3**; `GND` → **GND**; `L/R` → **GND** |
| **MAX98357A amplifier** | `BCLK` → **GPIO21**; `LRC` → **GPIO47**; `DIN` → **GPIO42** | `VIN` → **5V**; `GND` → **GND**; **leave `GAIN` and `SD` disconnected** |
| **Speaker (4 Ω / 3 W)** | Speaker `+` → amplifier **OUT+**; speaker `−` → amplifier **OUT−** | **Never connect either speaker lead to GND** |

**Important wiring notes**

- **I2S clock sharing:** the microphone and amplifier use the same **GPIO21 (BCLK/SCK)** and **GPIO47 (WS/LRC)**. Microphone data enters through **GPIO41**; amplifier data leaves through **GPIO42**.
- **MAX98357A header order (HW04 reference):** `LRC, BCLK, DIN, GAIN, SD, GND, VIN`. Check the markings on your actual amplifier board; do not assume every breakout uses this order.
- **Disconnect USB/power** before connecting or adjusting modules, and verify their voltage ratings before powering on.

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

## Reserved pins — do not repurpose for other accessories

These restrictions apply to the **JRBOT-HW-04** reference design:

| Pins | Reserved use / warning |
| --- | --- |
| **GPIO26–37** | **Flash/PSRAM:** keep reserved; GPIO35–37 are used by octal PSRAM on N16R8. |
| **GPIO0, 3, 45, 46** | **Boot/strapping:** do not reuse for new HW04 accessories without hardware review. |
| **GPIO19, 20** | **Native USB:** do not repurpose while using the board's USB functions. |
| **GPIO43, 44** | **UART0:** preserve for Serial/diagnostic functions in the HW04 reference. |
| **GPIO39, 40** | **JTAG/debug:** not approved for the proposed motor controls. |
| **Camera, OLED and I2S GPIOs listed above** | Already allocated: do not reuse for another module without a pin-policy review. |

### Hardware validation and additional cautions
- The current firmware includes I2S microphone RX and OLED support. Earlier notes stating otherwise describe superseded diagnostic revisions.
- V1.7.04 validated camera, microphone, playback and local WebRTC on HW04 and introduced I2S resource arbitration. These records do not replace a consolidated V1.8S release test.
- Motors and servos require their **own driver and power design**. Proposed motion controls are tracked in the [Roadmap](../01-history-roadmap/ROADMAP.md), not defined by this wiring map.

**Sources of truth:** [`hardware/pinmap.json`](../../hardware/pinmap.json), [generated firmware definitions](../../firmware/core/jr_pinmap_fixed.h), [pin policy tests](../../tests/hardware/test_pin_policy.py), and the [original assembly confirmation](../../hardware/CONFIRMAR_MONTAGEM.txt).

For a practical build sequence, see the [Assembly Guide](ASSEMBLY_GUIDE.md).
