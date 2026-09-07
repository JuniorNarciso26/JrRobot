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

## Memoria do ESP32-S3 N16R8

Na variante ESP32-S3-WROOM-1-N16R8, a memoria e:

- 16 MB de Flash Quad SPI.
- 8 MB de PSRAM Octal SPI.

Pinos ligados/reservados para memoria no ESP32-S3:

- GPIO26 a GPIO32: barramento SPI0/1 usado normalmente por Flash/PSRAM e nao deve ser reutilizado.
- GPIO33 a GPIO37: linhas adicionais usadas quando existe memoria Octal.
- No modulo N16R8, GPIO35, GPIO36 e GPIO37 ficam ligados internamente a PSRAM Octal e nao estao disponiveis para uso externo.

GPIO39 e GPIO40 **nao sao pinos de Flash/PSRAM** na documentacao oficial:

- GPIO39 = MTCK / JTAG, alem de GPIO normal e funcoes alternativas.
- GPIO40 = MTDO / JTAG, alem de GPIO normal e funcoes alternativas.

Mesmo nao sendo memoria, no JrBot vamos **evitar GPIO39 e GPIO40 para motores**, porque ja apresentaram comportamento estranho nos testes e pertencem ao grupo de debug/JTAG.

## Etapa futura - movimento do robo

A reserva dos GPIOs foi revisada contra a pinagem atual do JrBot e as restricoes oficiais do ESP32-S3.

| GPIO | Situacao | Uso futuro planejado |
|---:|---|---|
| 14 | **LIVRE / recomendado** | Movimento da cabeca |
| 38 | **LIVRE / recomendado** | Tracao / roda esquerda |
| 39 | **EVITAR** | JTAG MTCK; nao usar como pino principal |
| 40 | **EVITAR** | JTAG MTDO; nao usar como pino principal |
| 48 | **Candidato ao terceiro sinal** | Tracao / roda direita, se estiver fisicamente livre na placa |

### Reserva atual

- **Reservados desde ja:** GPIO14 e GPIO38.
- **Terceiro sinal de movimento:** preferir GPIO48 somente depois de confirmar que a placa fisica nao usa GPIO48 para LED RGB ou outro circuito onboard.
- **GPIO39 e GPIO40:** manter livres e evitar para motores por causa do JTAG e do comportamento observado nos testes.
- Se GPIO48 estiver ocupado ou nao estiver acessivel no conector da placa, estudar driver/expansor externo antes de sacrificar USB, UART, camera, OLED, audio ou microfone.
- Os GPIOs de movimento serao apenas sinais de controle; motores/servos nao devem ser alimentados pelo 3V3 da ESP32.

Objetivo futuro:

- 1 atuador para girar a cabeca do JrBot.
- 2 atuadores independentes para as rodas, permitindo avancar, recuar e girar.

Status: **planejamento futuro; nenhuma funcao de motor esta habilitada no firmware atual.**
