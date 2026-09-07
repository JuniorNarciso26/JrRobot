# JrBot - esquema de ligacao HW04 final

## MAX98357A - conector de 7 pinos

| Ordem | Pino | Ligar em |
|---:|---|---|
| 1 | LRC | GPIO47 |
| 2 | BCLK | GPIO21 |
| 3 | DIN | GPIO42 |
| 4 | GAIN | **NAO LIGAR** |
| 5 | SD | **NAO LIGAR** |
| 6 | GND | GND |
| 7 | VIN | 5V |

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

## OLED SSD1306 128x64

| OLED | ESP32-S3 |
|---|---:|
| SDA | GPIO1 |
| SCL | GPIO2 |
| VCC | 3V3 |
| GND | GND |

Se o OLED estiver desconectado, o JrBot continua funcionando e tenta reconhece-lo novamente.

## Camera OV5640

A camera permanece no conector original da placa. `Atualizar estado` faz uma verificacao do sensor e libera o teste quando houver resposta.

## Comportamento do painel

- Camera: libera teste apenas quando detectada.
- Microfone: libera teste apenas quando houver atividade I2S valida.
- OLED: libera os rostos quando o display responder.
- MAX98357A: teste fica disponivel porque o amplificador nao possui linha de retorno/ACK para confirmar presenca fisica.

## USB

- COM4: painel/comandos.
- COM6: gravacao.
