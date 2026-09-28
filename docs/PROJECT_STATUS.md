# Estado do projeto

Atualização: 2026-09-28.

## Hardware de referência

`JRBOT-HW-04`

- ESP32-S3 N16R8
- OLED SSD1306
- OV5640
- MAX98357A
- MS3625

## Direção atual

O projeto está organizado em quatro etapas de produto:

1. JrBot V1: painel interno pelo IP.
2. JrBot V2: comandos por voz.
3. JrBot V3: controle programático por API.
4. JrBrain: memória e comportamento avançado.

O detalhamento oficial está em [ROADMAP.md](ROADMAP.md).

## Etapa ativa

A versão oficial promovida em `main` é **`JrBot_V1.7.04`**. A `develop` parte da mesma baseline para as próximas entregas validadas.

### O que a V1.7.04 consolida

- Live WebRTC local full-duplex com PCMA/G.711A e JPEG/DataChannel;
- I2S RX/TX simultâneo no HW04 com BCLK GPIO21 e WS GPIO47 compartilhados;
- câmera OV5640 preservada na orientação física de -90°/270°;
- redução do polling automático recorrente de `/status` no App;
- `jr_mode_manager` como primeira autoridade central de modo;
- `jr_resource_manager` como primeira autoridade central de ownership do I2S;
- owners validados: `mic`, `playback` e `live`;
- telemetria de acquire/release/busy/mismatch do I2S.

Validação física final confirmada:

- `idle -> live -> idle`;
- gravação pelo MS3625;
- reprodução pelo MAX98357A;
- Live WebRTC conectando e encerrando;
- vídeo JPEG/DataChannel transmitindo;
- owner I2S retornando para `none`;
- `i2s_release_mismatch=0`;
- sem reset/watchdog observado.

Limitações conhecidas que não bloqueiam esta release:

- eco acústico/AEC ainda não tratado;
- eventos ocasionais `VIDEO_DC_DROP buffer_full`;
- Live pela Internet ainda não implementada;
- ESP-SR/MultiNet e o modo autônomo pertencem à V2 de voz.

O nome experimental usado durante a derivação da revisão não é o nome oficial da release. A identificação pública correta é `JrBot_V1.7.04`.

## Nomenclatura

- JrBot V1, V2 e V3: versões do produto.
- Runtime API 1.x: versão do protocolo.
- Revisões da V1 seguem o padrão `JrBot_V1.x.y` quando forem correções/refinamentos incrementais.

## Baseline técnica promovida

A baseline oficialmente promovida em `main` é `JrBot_V1.7.04`. A `develop` parte da mesma revisão até receber novas entregas validadas.

O reconhecimento de voz MultiNet6 continua experimental e pertence à frente da JrBot V2.

## Instalador

O `INSTALAR.bat` consulta o GitHub e monta o menu a partir das branches remotas ativas. Branches `archive/*` não são exibidas.

O instalador pode sincronizar a branch selecionada, compilar, gravar o ESP32 e abrir o painel de desenvolvimento.

## Branches

- `main`: última versão oficialmente promovida.
- `develop`: integração das entregas validadas.
- `v1`: referência da linha JrBot V1.
- `v2`: JrBot V2 voz.
- `feature/*`: desenvolvimento temporário.
- `fix/*` / `hotfix/*`: correções temporárias.
- `archive/*`: histórico, oculto do instalador.

## Regra de promoção e documentação

Sempre que uma versão for promovida para `develop`, devem ser atualizados no mesmo ciclo:

1. marcador de versão do firmware;
2. `README.md`;
3. `docs/ROADMAP.md`;
4. `docs/PROJECT_STATUS.md`;
5. `docs/CHANGELOG.md`;
6. documentação técnica afetada pela mudança.

Implementação, build, teste de bancada e validação física continuam documentados separadamente.
