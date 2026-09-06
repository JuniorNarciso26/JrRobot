# JrBot - pinagem HW04 final

Firmware: `JRBotV2_2026-09-06-07:10`.

## Audio I2S

| GPIO | Funcao |
|---:|---|
| 21 | BCLK/SCK compartilhado entre MAX98357A e MS3625 |
| 47 | WS/LRC compartilhado entre MAX98357A e MS3625 |
| 42 | ESP32 DOUT -> DIN do MAX98357A |
| 41 | SD do MS3625 -> ESP32 |

### MAX98357A - ordem dos 7 pinos

1. `LRC` -> GPIO47
2. `BCLK` -> GPIO21
3. `DIN` -> GPIO42
4. `GAIN` -> **nao ligar**
5. `SD` -> **nao ligar**
6. `GND` -> GND
7. `VIN` -> 5V

### MS3625

- SCK -> GPIO21
- WS -> GPIO47
- SD -> GPIO41
- VDD -> 3V3
- GND -> GND
- L/R -> GND

## OLED SSD1306

- SDA -> GPIO1
- SCL -> GPIO2
- VCC -> 3V3
- GND -> GND

## Camera OV5640

| Sinal | GPIO |
|---|---:|
| XCLK | 15 |
| SCCB SDA/SCL | 4/5 |
| D0-D3 | 11/9/8/10 |
| D4-D7 | 12/18/17/16 |
| VSYNC/HREF/PCLK | 6/7/13 |
| PWDN/RESET | -1/-1 |

## USB

- COM4: comandos/painel.
- COM6: gravacao.
