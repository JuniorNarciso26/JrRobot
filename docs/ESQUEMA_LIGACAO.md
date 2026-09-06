# JrBot - esquema de ligacao HW04

## Amplificador MAX98357A - conector de 7 pinos

Use a **ordem impressa no modulo mostrado**, para ficar facil localizar cada pino:

| Ordem | Pino MAX98357A | Ligar em |
|---:|---|---|
| 1 | LRC | GPIO47 |
| 2 | BCLK | GPIO21 |
| 3 | DIN | GPIO42 |
| 4 | GAIN | **NAO LIGAR** nesta montagem |
| 5 | SD | **NAO LIGAR** nesta montagem |
| 6 | GND | GND da ESP32 |
| 7 | VIN | 5V da placa, usando a alimentacao USB desta montagem |

O falante 4 ohms / 3 W liga somente entre `OUT+` e `OUT-` do MAX98357A. Nenhum terminal do falante vai ao GND.

## Microfone MS3625 I2S

| MS3625 | ESP32-S3 |
|---|---:|
| SCK | GPIO21 |
| WS | GPIO47 |
| SD | GPIO41 |
| VDD | 3V3 |
| GND | GND |
| L/R | GND |

GPIO21 e GPIO47 sao compartilhados entre microfone e amplificador. GPIO41 e entrada do microfone; GPIO42 e saida para o amplificador.

## OLED SSD1306 128x64

| OLED | ESP32-S3 |
|---|---:|
| SDA | GPIO1 |
| SCL | GPIO2 |
| GND | GND |
| VCC | 3V3 nesta montagem |

O firmware nao para se o OLED estiver ausente. Ele continua tentando novamente; se o OLED for conectado depois, `Atualizar estado` passa a libera-lo quando o driver confirmar resposta.

## Camera OV5640

A camera permanece no conector original da placa.

| Sinal | GPIO |
|---|---:|
| XCLK | 15 |
| SDA/SCL | 4/5 |
| D0-D3 | 11/9/8/10 |
| D4-D7 | 12/18/17/16 |
| VSYNC/HREF/PCLK | 6/7/13 |

Ao clicar `Atualizar estado`, o firmware tenta identificar a OV5640. Se nao responder, o botao de teste fica bloqueado; ao conectar e atualizar novamente, o teste pode ser liberado.

## USB

- COM6: gravacao.
- COM4: painel e comandos.
- Nao abrir outro monitor na COM4 junto com o painel.

Antes de alterar fios, desligue as duas USBs e qualquer fonte externa.
