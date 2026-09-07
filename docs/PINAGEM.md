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

Reservar tres GPIOs exclusivamente para a futura mecanica de movimento. Eles nao fazem parte da Etapa 1 e nao devem ser usados por novas funcoes sem revisar esta reserva.

| GPIO reservado | Uso futuro planejado |
|---:|---|
| 14 | Movimento da cabeca |
| 38 | Tracao / roda esquerda |
| 39 | Tracao / roda direita |

Objetivo futuro:

- 1 atuador para girar a cabeca do JrBot.
- 2 atuadores independentes para as rodas, permitindo avancar, recuar e girar.
- Os tres GPIOs ficam reservados somente para sinais de controle; motores/servos nao devem ser alimentados pelo 3V3 da ESP32.
- Antes da implementacao mecanica, confirmar que GPIO14, GPIO38 e GPIO39 estao fisicamente acessiveis na placa usada no robo.
- Se a placa final nao disponibilizar os tres sinais, estudar um driver/expansor externo para preservar a pinagem atual de camera, OLED, audio e microfone.

Status: **planejamento futuro; nenhuma funcao de motor esta habilitada no firmware atual.**
