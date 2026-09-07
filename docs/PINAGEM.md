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

## Etapa futura - movimento do robo

A reserva dos GPIOs foi revisada contra a pinagem atual do JrBot e as restricoes oficiais do ESP32-S3.

| GPIO | Situacao | Uso futuro planejado |
|---:|---|---|
| 14 | **LIVRE / recomendado** | Movimento da cabeca |
| 38 | **LIVRE / recomendado** | Tracao / roda esquerda |
| 39 | **NAO reservar como pino principal** | Candidato apenas se necessario |
| 48 | **Candidato preferencial ao terceiro sinal** | Tracao / roda direita, se estiver fisicamente livre na placa |

### Motivo da revisao

- GPIO14 nao e usado por camera, OLED, audio, microfone, USB, UART0 ou memoria do JrBot. No ESP32-S3 ele e GPIO de prioridade P2, sem restricao especial para uso normal.
- GPIO38 tambem nao e usado pelo hardware atual. Ele fica fora dos GPIO26-37 reservados para flash/PSRAM e e GPIO P2 no ESP32-S3.
- GPIO39 nao e usado atualmente pelo JrBot, mas pertence ao grupo JTAG MTCK do ESP32-S3. A Espressif classifica GPIO39-42 como pinos que exigem cautela. Como o JrBot ja usa GPIO41/42 para audio, nao devemos criar outra dependencia futura nesse grupo sem necessidade.
- GPIO48 e P2 no ESP32-S3 e nao aparece na pinagem funcional atual do JrBot. Porem, antes de reserva-lo definitivamente, deve ser confirmado se a placa fisica nao usa GPIO48 para LED RGB ou outro circuito onboard.

### Reserva atual

- **Reservados desde ja:** GPIO14 e GPIO38.
- **Terceiro sinal de movimento:** preferir GPIO48 somente depois de confirmar a placa fisica.
- **GPIO39:** manter livre e evitar como escolha principal por causa do JTAG.
- Se GPIO48 estiver ocupado ou nao estiver acessivel no conector da placa, estudar driver/expansor externo antes de sacrificar USB, UART, camera, OLED, audio ou microfone.
- Os GPIOs de movimento serao apenas sinais de controle; motores/servos nao devem ser alimentados pelo 3V3 da ESP32.

Objetivo futuro:

- 1 atuador para girar a cabeca do JrBot.
- 2 atuadores independentes para as rodas, permitindo avancar, recuar e girar.

Status: **planejamento futuro; nenhuma funcao de motor esta habilitada no firmware atual.**
