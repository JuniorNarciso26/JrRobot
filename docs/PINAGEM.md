# JrBot - pinagem HW04

Fonte dos GPIOs: `hardware/pinmap.json`.

## Audio I2S

| GPIO | Funcao |
|---:|---|
| 21 | BCLK/SCK compartilhado: MS3625 + MAX98357A |
| 47 | WS/LRC compartilhado: MS3625 + MAX98357A |
| 41 | SD do MS3625 -> ESP32 |
| 42 | ESP32 DOUT -> DIN do MAX98357A |

### MAX98357A - ordem dos 7 pinos

`LRC -> BCLK -> DIN -> GAIN -> SD -> GND -> VIN`

- LRC = GPIO47
- BCLK = GPIO21
- DIN = GPIO42
- GAIN = nao ligar
- SD = nao ligar
- GND = GND
- VIN = 5V

## OLED

- GPIO1 = SDA SSD1306
- GPIO2 = SCL SSD1306
- A ausencia do OLED nao bloqueia o JrBot; o driver tenta recuperar e permite conexao posterior.

## Camera OV5640

| Sinal | GPIO |
|---|---:|
| XCLK | 15 |
| SCCB SDA/SCL | 4/5 |
| D0-D3 | 11/9/8/10 |
| D4-D7 | 12/18/17/16 |
| VSYNC/HREF/PCLK | 6/7/13 |
| PWDN/RESET | -1 / -1 |

## Reservas

- GPIO19/20: USB nativa.
- GPIO43/44: UART0 / painel nesta montagem.
- GPIO26-37: reservados para flash/PSRAM N16R8.
- GPIO0/3/45/46: boot/strapping; nao realocar.
- COM6: gravacao; COM4: painel/comandos.

O MS3625 ja tem pinagem reservada, mas o driver de captura ainda nao esta implementado.
