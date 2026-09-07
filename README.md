# JrBot - robo com IA

Branch ativa: `v2-revisada`.

**Firmware atual:** `JRBotV2_2026-09-06-07:10`  
**Hardware:** `JRBOT-HW-04`  
**Perfil:** `full_hardware_test`

## Hardware liberado para teste

- OLED SSD1306 128x64: SDA GPIO1, SCL GPIO2. Se estiver ausente, o robo continua e o driver tenta recuperar.
- Camera OV5640: verificada ao usar `Atualizar estado`; o teste libera quando a camera responde.
- Amplificador MAX98357A: BCLK GPIO21, LRC GPIO47, DIN GPIO42. Presenca fisica nao pode ser detectada; o teste de som fica disponivel.
- Microfone MS3625: SCK GPIO21, WS GPIO47, SD GPIO41. `Atualizar estado` le amostras I2S e libera o teste quando ha atividade digital.

MAX98357A, ordem dos 7 pinos: `LRC, BCLK, DIN, GAIN, SD, GND, VIN`. Nesta montagem `GAIN` e `SD` ficam sem ligar.

## Instalar

No terminal ESP-IDF 5.5.x:

```bat
git switch v2-revisada
git pull --ff-only origin v2-revisada
INSTALAR.bat
```

O instalador usa uma configuracao final nova, compila, grava pela COM6 e abre o painel. No seletor do painel aparecem somente COM4 e COM6.

Documentos: [pinagem](docs/PINAGEM.md), [esquema de ligacao](docs/ESQUEMA_LIGACAO.md) e [painel](PAINEL_V2.md).
