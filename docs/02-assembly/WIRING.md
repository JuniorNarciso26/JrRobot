# JrBot Wiring and Hardware Reference — HW04

**JRBOT-HW-04** uses an ESP32-S3 N16R8 (16 MB Flash, 8 MB PSRAM), OV5640 camera, SSD1306 OLED, MS3625 microphone and MAX98357A amplifier. This file is the **single human-readable reference** for HW04 electrical connections and constraints.

## Electrical connections

Connect each accessory according to its own pin list. **Disconnect power before wiring.** Check the printed pin labels on your modules.

### 1. OLED — SSD1306

| OLED pin | Connect to |
| --- | --- |
| `SDA` | **GPIO1** |
| `SCL` | **GPIO2** |
| `VCC` | **3V3** |
| `GND` | **GND** |

### 2. Microphone — MS3625 (I2S)

| Microphone pin | Connect to |
| --- | --- |
| `SCK` | **GPIO21** |
| `WS` | **GPIO47** |
| `SD` | **GPIO41** |
| `VDD` | **3V3** |
| `GND` | **GND** |
| `L/R` | **GND** |

### 3. Amplifier — MAX98357A (I2S)

| Amplifier pin | Connect to |
| --- | --- |
| `LRC` | **GPIO47** |
| `BCLK` | **GPIO21** |
| `DIN` | **GPIO42** |
| `GAIN` | **Not connected** |
| `SD` | **Not connected** |
| `GND` | **GND** |
| `VIN` | **5V** |

**HW04 seven-pin order:** `LRC → BCLK → DIN → GAIN → SD → GND → VIN`. Use the pin labels on your actual board; layouts can vary.

### 4. Speaker — 4 Ω / 3 W

| Speaker terminal | Connect to |
| --- | --- |
| `+` | Amplifier **OUT+** |
| `−` | Amplifier **OUT−** |

**Warning:** Never connect either speaker terminal to GND.

### Shared I2S signals

The microphone and amplifier share **GPIO21 (BCLK/SCK)** and **GPIO47 (WS/LRC)**. The microphone sends data through **GPIO41**, while the amplifier receives data through **GPIO42**. This sharing is intentional in HW04.

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
