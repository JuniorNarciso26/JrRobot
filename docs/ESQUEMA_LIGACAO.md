# JrBot - esquema de ligacao HW04

## Audio

### Microfone MS3625 I2S

| MS3625 | ESP32-S3 |
|---|---:|
| SCK | GPIO21 |
| WS | GPIO47 |
| SD | GPIO41 |
| VDD | 3V3 |
| GND | GND |
| L/R | GND para o canal selecionado nesta configuracao |

### Amplificador MAX98357A

| MAX98357A | ESP32-S3 |
|---|---:|
| BCLK | GPIO21 |
| LRC/WS | GPIO47 |
| DIN | GPIO42 |
| GND | GND comum |
| VIN | Alimentacao compativel com o breakout; confirmar antes de energizar |

O falante 4 ohms / 3 W liga somente entre `OUT+` e `OUT-` do MAX98357A. Nenhum terminal do falante vai ao GND.

GPIO21 e GPIO47 sao compartilhados entre microfone e amplificador. GPIO41 e somente dado do microfone. GPIO42 e somente dado para o amplificador.

## OLED SSD1306 128x64

| OLED | ESP32-S3 |
|---|---:|
| SDA | GPIO1 |
| SCL | GPIO2 |
| GND | GND |
| VCC | Conforme o breakout; logica I2C deve permanecer compativel com 3,3 V |

O OLED pode ficar desconectado durante os testes; o firmware headless nao depende dele.

## Camera OV5640

A camera permanece no conector da placa. Nao refazer o cabo flex.

| Sinal | GPIO |
|---|---:|
| XCLK | 15 |
| SDA/SCL | 4/5 |
| D0-D3 | 11/9/8/10 |
| D4-D7 | 12/18/17/16 |
| VSYNC/HREF/PCLK | 6/7/13 |

## USB

- COM6: gravacao da placa nesta montagem.
- COM4: painel e comandos nesta montagem.
- Nao abrir outro monitor na COM4 ao mesmo tempo que o painel.

Antes de alterar fios, desligue as duas USBs e qualquer fonte externa.
