# JrBot - robo com IA

JrBot e um robo com IA em desenvolvimento baseado em ESP32-S3, com painel no computador, camera OV5640, audio I2S e rosto OLED SSD1306 opcional.

Branch ativa: `v2-revisada`. Firmware: **JRBOT-V2-DIAG-03 / JRBOT-HW-04**. Build desta revisao: **2026-09-06 06:44 BRT (America/Sao_Paulo)**.

## Hardware

- ESP32-S3-N16R8 dual USB-C.
- Camera OV5640.
- OLED SSD1306 I2C 128x64.
- Microfone MS3625 I2S.
- Amplificador MAX98357A I2S.
- Falante 4 ohms / 3 W.

## Audio HW04

| GPIO | Uso |
|---:|---|
| 21 | BCLK/SCK compartilhado |
| 47 | WS/LRC compartilhado |
| 41 | SD do microfone -> ESP32 |
| 42 | ESP32 -> DIN do MAX98357A |

MAX98357A, ordem do conector de 7 pinos: **LRC, BCLK, DIN, GAIN, SD, GND, VIN**. Nesta montagem **GAIN e SD ficam sem ligar**.

## Atualizar estado

O firmware trata os modulos de forma independente:

- OLED: falha nao para o robo; o driver tenta novamente e pode reconhecer um OLED conectado depois.
- Camera: `Atualizar estado` faz uma verificacao da OV5640; o teste fica disponivel somente quando ela responde.
- MAX98357A: nao possui retorno de presenca; o painel pode liberar o teste quando o I2S esta configurado, mas somente ouvir o som confirma o conjunto fisico.
- MS3625: pinagem definida; driver RX ainda pendente.

## Instalar

No terminal ESP-IDF 5.5.x:

```bat
git pull --ff-only origin v2-revisada
INSTALAR.bat
```

COM6 grava. COM4 e usada pelo painel. Nao e necessario monitor Serial separado.

Documentos: [pinagem](docs/PINAGEM.md), [esquema](docs/ESQUEMA_LIGACAO.md), [materiais](docs/MATERIAIS.md) e [painel](PAINEL_V2.md).
