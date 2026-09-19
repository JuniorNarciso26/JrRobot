# Estado do projeto

Atualização: 2026-09-19.

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

A revisão atual é **`JrBot_V1.6.2`**, promovida como manutenção da `JrBot_V1.6.1`.

### O que mudou na V1.6.2

- `INSTALAR.bat` consulta dinamicamente as branches remotas ativas;
- branches de desenvolvimento/correção podem ser selecionadas pelo instalador;
- `archive/*` permanece fora do menu;
- a branch atual é identificada no seletor;
- o fluxo de sincronização, compilação, gravação e abertura do painel continua centralizado no instalador.

A V1.6.2 não altera o PTT half-duplex nem o caminho de áudio. O estudo de comunicação de voz ao vivo/full-duplex permanece separado.

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
