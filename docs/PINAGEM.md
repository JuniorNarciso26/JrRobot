# JrBot - pinagem HW04

Fonte dos pinos: `hardware/pinmap.json`.

## Audio I2S

| GPIO | Funcao |
|---:|---|
| 21 | BCLK/SCK compartilhado: MS3625 + MAX98357A |
| 47 | WS/LRCLK compartilhado: MS3625 + MAX98357A |
| 41 | Entrada no ESP32: SD do microfone MS3625 |
| 42 | Saida do ESP32: DOUT -> DIN do MAX98357A |

O microfone e o amplificador compartilham apenas BCLK e WS. As linhas de dados ficam separadas.

## OLED

| GPIO | Funcao |
|---:|---|
| 1 | SDA SSD1306 |
| 2 | SCL SSD1306 |

O OLED continua desabilitado no perfil `headless_diagnostic`.

## Camera OV5640

| Sinal | GPIO |
|---|---:|
| XCLK | 15 |
| SCCB SDA | 4 |
| SCCB SCL | 5 |
| D0/D1/D2/D3 | 11/9/8/10 |
| D4/D5/D6/D7 | 12/18/17/16 |
| VSYNC/HREF/PCLK | 6/7/13 |
| PWDN/RESET | -1 / -1 |

## Reservas da placa/projeto

- GPIO19/20: USB nativa.
- GPIO43/44: UART0 usada pelo painel/USB-UART nesta montagem.
- GPIO26-37: reservados pela politica do projeto para flash/PSRAM N16R8.
- GPIO0/3/45/46: nao realocar nesta revisao por boot/strapping.
- COM6: gravacao; COM4: painel/comandos, conforme a montagem atual.

O driver de entrada do MS3625 ainda sera implementado. A pinagem acima ja fica reservada para impedir conflito futuro.
