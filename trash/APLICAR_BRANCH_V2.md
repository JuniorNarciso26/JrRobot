# Aplicar a branch v2-revisada

Revisao atual: **JRBOT-V2-DIAG-03 / JRBOT-HW-04**.

## Fluxo padrao

No terminal ESP-IDF 5.5.x:

```bat
git switch v2-revisada
git pull --ff-only origin v2-revisada
INSTALAR.bat
```

`INSTALAR.bat` compila, grava pela **COM6** e abre o painel. No painel, use **COM4** para comandos nesta montagem.

## Comandos opcionais

```bat
INSTALAR.bat build
INSTALAR.bat flash
INSTALAR.bat panel
INSTALAR.bat menuconfig
```

Para usar outra porta de gravacao:

```bat
set JR_FLASH_PORT=COM7
INSTALAR.bat flash
```

## Pinagem HW04

- GPIO21: BCLK/SCK compartilhado entre microfone e amplificador.
- GPIO47: WS/LRC compartilhado.
- GPIO41: SD do microfone MS3625 para o ESP32.
- GPIO42: DOUT do ESP32 para DIN do MAX98357A.
- OLED: SDA GPIO1 / SCL GPIO2.
- Camera: OV5640 no conector da placa.

O microfone ja tem a pinagem reservada, mas o driver de captura ainda nao esta implementado.
