# JrBot — Final HW04 Pin Mapping

Reference baseline: **`JrBot_V1.7.04` / `JRBOT-HW-04`**.

## I2S audio

| GPIO | Function |
| ---: | --- |
| 21 | BCLK/SCK shared by MAX98357A and MS3625 |
| 47 | WS/LRC shared by MAX98357A and MS3625 |
| 42 | ESP32 DOUT → MAX98357A DIN |
| 41 | MS3625 SD → ESP32 |

### MAX98357A — seven-pin order

1. `LRC` → GPIO47
2. `BCLK` → GPIO21
3. `DIN` → GPIO42
4. `GAIN` → **leave disconnected**
5. `SD` → **leave disconnected**
6. `GND` → GND
7. `VIN` → 5V

### MS3625

- SCK → GPIO21
- WS → GPIO47
- SD → GPIO41
- VDD → 3V3
- GND → GND
- L/R → GND

## SSD1306 OLED

- SDA → GPIO1
- SCL → GPIO2
- VCC → 3V3
- GND → GND

## OV5640 camera

| Signal | GPIO |
| --- | ---: |
| XCLK | 15 |
| SCCB SDA/SCL | 4/5 |
| D0–D3 | 11/9/8/10 |
| D4–D7 | 12/18/17/16 |
| VSYNC/HREF/PCLK | 6/7/13 |
| PWDN/RESET | -1/-1 |

## USB

- **COM4:** commands/panel.
- **COM6:** flashing.

## ESP32-S3 N16R8 memory

The ESP32-S3-WROOM-1-N16R8 variant has:

- 16 MB Quad SPI Flash.
- 8 MB Octal SPI PSRAM.

Pins connected to or reserved for the ESP32-S3 memory:

- **GPIO26–GPIO32:** SPI0/1 bus typically used by Flash/PSRAM and must not be repurposed.
- **GPIO33–GPIO37:** additional lines used for Octal memory.
- On the N16R8 module, **GPIO35, GPIO36, and GPIO37** are internally connected to Octal PSRAM and are unavailable externally.

**GPIO39 and GPIO40 are not Flash/PSRAM pins** according to the chip documentation:

- GPIO39 = MTCK/JTAG, also capable of normal GPIO and alternate functions.
- GPIO40 = MTDO/JTAG, also capable of normal GPIO and alternate functions.

Even though they are not memory pins, JrBot avoids GPIO39 and GPIO40 for motors because unexpected behavior was observed during tests and both pins belong to the debug/JTAG group.

## Future development — robot movement

Candidate GPIOs were reviewed against JrBot's existing pin map and the ESP32-S3 restrictions.

| GPIO | Assessment | Proposed future use |
| ---: | --- | --- |
| 14 | **Available / recommended** | Head movement |
| 38 | **Available / recommended** | Left-wheel drive |
| 39 | **Avoid** | MTCK/JTAG; not a preferred control pin |
| 40 | **Avoid** | MTDO/JTAG; not a preferred control pin |
| 48 | **Candidate third signal** | Right-wheel drive, provided it is available on the physical board |

### Current reservation

- **Reserved:** GPIO14 and GPIO38.
- **Third movement signal:** GPIO48 is preferred only after confirming the physical board does not use it for an RGB LED or another onboard circuit.
- **GPIO39 and GPIO40:** leave available and avoid for motor control because of JTAG and the test behavior.
- If GPIO48 is occupied or inaccessible, investigate an external motor driver or I/O expander before sacrificing USB, UART, camera, OLED, audio, or microphone pins.
- Movement GPIOs are **control signals only**; do not power motors or servos from the ESP32's 3V3 rail.

Future goal: one actuator for head rotation and two independent wheel actuators for forward, reverse, and turning motions.

**Status:** future planning only. No motor-control functionality is enabled in the current firmware.
