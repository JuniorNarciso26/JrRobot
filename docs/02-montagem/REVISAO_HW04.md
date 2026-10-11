# JrBot — revisão HW04

A **JRBOT-HW-04** é a referência física atual do projeto.

## Componentes confirmados

- ESP32-S3 N16R8;
- câmera OV5640;
- OLED SSD1306;
- microfone MS3625 I2S;
- amplificador MAX98357A.

## Áudio I2S

Pinagem adotada e fisicamente utilizada:

```text
BCLK / SCK     GPIO21
WS / LRCLK     GPIO47
MS3625 SD      GPIO41
MAX98357A DIN  GPIO42
```

BCLK e WS são propositalmente compartilhados entre o microfone RX e o amplificador TX.

A baseline estável `JrBot_V1.7.04` utiliza RX/TX I2S simultâneos para a Live WebRTC e possui gerenciamento central de ownership do recurso I2S.

## Câmera

Câmera: **OV5640**.

Orientação física confirmada no hardware atual:

```text
-90° / 270°
```

Essa orientação deve ser preservada em firmware normal, salvo em experimento específico de câmera.

## OLED

Display: **SSD1306**, I2C em GPIO1/GPIO2 conforme a pinagem oficial.

## Fonte de verdade

Para mudanças de hardware consultar em conjunto:

- `hardware/pinmap.json`;
- [PINAGEM.md](PINAGEM.md);
- [ESQUEMA_LIGACAO.md](ESQUEMA_LIGACAO.md);
- testes/políticas de GPIO.

Documentos de HW03 e perfis diagnósticos antigos foram movidos para `trash/`.
