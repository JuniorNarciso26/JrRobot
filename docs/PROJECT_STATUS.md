# Estado do projeto

Atualização da integração documental: 2026-10-10.

## Hardware de referência

**JRBOT-HW-04**

- ESP32-S3 N16R8
- OLED SSD1306
- OV5640
- MAX98357A
- MS3625

## Release oficial

A referência de release estável fisicamente validada permanece **`JrBot_V1.7.04`**. A linha JrSkill adiciona o build `JrBot_V1S_APP_03`, validado para o MVP na **Solana Devnet**, mas sua incorporação ao código não deve ser confundida com uma nova certificação de firmware estável ou operação Mainnet.

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

Referência oficial de release estável local: `JrBot_V1.7.04`. A integração do código V1S é um marco separado, validado no escopo do hackathon.

### `develop`

Integração de trabalho já validado antes de uma nova promoção.

### `V1s-00` — JrSkill Network

**MVP do Colosseum concluído em 2026-10-10.** A linha JrSkill Network implementou criação e publicação pela Store, Wallet/Phantom, Skill/Offer/License PDA em Devnet, descoberta e execução física de Skills limitadas pelas capabilities do JrBot. Build de referência: `JrBot_V1S_APP_03`; validação física da segunda Skill registrada em 07/10/2026.

O histórico, Day 1–9, transações e screenshots ficam em [HACKATHON_DEVLOG.md](HACKATHON_DEVLOG.md) e também na branch original `V1s-00`. A próxima etapa de produto será definida separadamente; não foi demonstrada Mainnet ou adoção global.

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
