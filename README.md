# JrBot - robo com IA

JrBot e um robo com IA em desenvolvimento baseado em ESP32-S3, com painel no computador, camera, audio I2S e rosto OLED opcional.

A implementacao atual esta na branch `v2-revisada`. Firmware desta revisao: **JRBOT-V2-DIAG-03 / JRBOT-HW-04**.

## Hardware confirmado

- ESP32-S3-N16R8 com duas USB-C.
- Camera OV5640.
- OLED SSD1306 I2C 128x64.
- Microfone MS3625 I2S.
- Amplificador MAX98357A I2S.
- Falante 4 ohms / 3 W.

## Pinagem de audio HW04

| GPIO | Uso |
|---:|---|
| 21 | BCLK/SCK compartilhado entre microfone e amplificador |
| 47 | WS compartilhado entre microfone e amplificador |
| 41 | SD do microfone -> ESP32 |
| 42 | ESP32 DOUT -> DIN do MAX98357A |

OLED: SDA GPIO1 / SCL GPIO2. A camera permanece no conector original da placa.

O driver de entrada do microfone ainda nao esta implementado. A camera e o amplificador continuam com testes sob demanda; nenhum deles inicia automaticamente no boot. A IA conversacional ainda e etapa de desenvolvimento.

## Instalar e operar

No terminal ESP-IDF 5.5.x:

```bat
git switch v2-revisada
git pull --ff-only origin v2-revisada
INSTALAR.bat
```

O instalador unico compila, grava pela **COM6** e abre o painel. No painel, use **COM4** para comandos nesta montagem. Nao e necessario monitor Serial separado.

Comandos opcionais: `INSTALAR.bat build`, `INSTALAR.bat flash`, `INSTALAR.bat panel` e `INSTALAR.bat menuconfig`.

Documentos: [instalacao](APLICAR_BRANCH_V2.md), [pinagem](docs/PINAGEM.md), [esquema](docs/ESQUEMA_LIGACAO.md), [materiais](docs/MATERIAIS.md) e [painel](PAINEL_V2.md).
