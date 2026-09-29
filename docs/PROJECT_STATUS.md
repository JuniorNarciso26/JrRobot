# Estado do projeto

Atualização: 2026-09-29.

## Hardware de referência

**JRBOT-HW-04**

- ESP32-S3 N16R8
- OLED SSD1306
- OV5640
- MAX98357A
- MS3625

## Release oficial

A versão oficialmente promovida em `main` é **`JrBot_V1.7.04`**.

A `develop` está baseada na mesma baseline.

A V1.7.04 consolida:

- controle local por navegador;
- Wi-Fi e HTTPS local;
- foto e Live com OV5640;
- áudio WebRTC bidirecional;
- JPEG por DataChannel;
- I2S full-duplex no HW04;
- `jr_mode_manager`;
- `jr_resource_manager`;
- ownership de I2S para mic, playback e Live.

Limitações conhecidas da release estável incluem AEC/eco acústico, acesso WebRTC pela Internet e eventos ocasionais de backpressure de vídeo. A linha de voz local/ESP-SR pertence à V2.

## Linhas oficiais abertas

### `main`

Release oficial estável: `JrBot_V1.7.04`.

### `develop`

Integração de trabalho já validado antes de uma nova promoção.

### `V1s-00` — JrSkill Network

Linha oficial paralela dedicada a **Skills portáveis para IA física**, com distribuição, versionamento e licenciamento apoiados por Solana e execução protegida por uma camada local de capabilities.

A documentação pública da `main` registra somente esse objetivo. O andamento e as etapas internas dessa linha pertencem à própria branch `V1s-00`.

### `v2`

Linha preservada para controle por voz local.

### `v1`

Referência histórica da linha V1. Não é a baseline estável atual.

## Hardware e recursos compartilhados

No HW04:

```text
BCLK / SCK     GPIO21
WS / LRCLK     GPIO47
MS3625 SD      GPIO41
MAX98357A DIN  GPIO42
```

BCLK e WS são intencionalmente compartilhados entre RX e TX.

## Instalador

O `INSTALAR.bat` consulta as branches remotas disponíveis, sincroniza a branch selecionada e pode compilar, gravar e abrir o painel.

Sempre confirme no log/status a identificação exata da build instalada.

## Regra de documentação

Ao promover uma nova baseline para `develop` ou `main`, revisar no mesmo ciclo:

1. `firmware/version.txt`;
2. `README.md`;
3. `docs/ROADMAP.md`;
4. `docs/PROJECT_STATUS.md`;
5. `docs/CHANGELOG.md`;
6. documentação técnica afetada.

Documentos antigos que não representam mais o estado vigente devem ser movidos para `trash/` antes de eventual exclusão.
