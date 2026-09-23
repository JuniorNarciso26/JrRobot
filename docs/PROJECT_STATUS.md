# Estado do projeto

Atualização: 2026-09-23.

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

A JrBot V1 está funcionalmente concluída e integrada em `develop`.

A revisão atual é **`JrBot_V1.6.3`**, com Live WebRTC local full-duplex validada no HW04.

### O que mudou na V1.6.3

- áudio WebRTC PCMA/G.711A bidirecional entre JrBot e navegador;
- vídeo OV5640 em JPEG pelo WebRTC DataChannel/SCTP;
- perfis rápido/QVGA, equilibrado/VGA e qualidade/SVGA;
- mute independente nas duas direções de áudio;
- I2S full-duplex real no ESP32-S3 usando BCLK/WS compartilhados;
- sinalização local HTTPS + SSE/POST;
- monitor de RAM interna e PSRAM durante a Live;
- política de alocação em PSRAM alinhada ao exemplo oficial `local_jpeg_stream`;
- orientação física da câmera preservada em -90°/270°.

Validação física confirmada:
- JrBot → celular audível;
- celular → JrBot audível;
- vídeo contínuo;
- mute do celular e mute do JrBot funcionais.

Limitações conhecidas:
- eco acústico/AEC ainda não tratado;
- foram observados eventos `VIDEO_DC_DROP buffer_full` sem perda perceptível no teste A/B;
- a Live é local; comunicação remota pela Internet continua como evolução da Issue #26.

O hotfix `JrBot_V1.6.2.1` de rotação ficou aberto fora de `develop`, mas sua correção está absorvida pela V1.6.3.

## Nomenclatura

- JrBot V1, V2 e V3: versões do produto.
- Runtime API 1.x: versão do protocolo.
- Revisões da V1 seguem o padrão `JrBot_V1.x.y` quando forem correções/refinamentos incrementais.

## Baseline técnica promovida

A baseline oficialmente promovida em `main` continua separada da integração em `develop`. A promoção de uma revisão para `main` exige validação própria.

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
