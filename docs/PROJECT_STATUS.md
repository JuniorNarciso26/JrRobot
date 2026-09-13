# Estado do projeto

Atualização: 2026-09-13.

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

A etapa ativa é a JrBot V1, na branch `feature/v1-internal-web-panel`, com PR #18 para `develop`.

O painel interno básico já foi exercitado no hardware real. Ainda fazem parte do fechamento da V1 o refinamento para celular, a interface da câmera e o envio de áudio do celular ou computador.

O `PAINEL.bat` continua sendo ferramenta de instalação, desenvolvimento e diagnóstico. O painel servido pelo ESP32 é a interface de uso da V1.

## Nomenclatura

- JrBot V1, V2 e V3: versões do produto.
- Runtime API 1.x: versão do protocolo.
- Identificadores como `_V1_02`, `_V1_03` e `_V1_04`: revisões técnicas de firmware.

Essas numerações são independentes.

## Baseline técnica promovida

A baseline promovida em `main` continua sendo `JRBotV2_RUNTIME_API_V1_02`, com Runtime API `1.1`.

Os testes já registrados incluem mudança de face, leitura de estados, HTTP local, microfone e áudio I2S Philips.

O reconhecimento de voz MultiNet6 continua experimental e pertence à frente da JrBot V2.

## Instalador

Na linha atual, `INSTALAR.bat` consulta o GitHub, carrega as branches remotas ativas, oculta `archive/*`, permite escolher a branch, sincroniza o projeto, compila, grava o ESP32 e abre o painel de desenvolvimento.

O painel de desenvolvimento pode ser encerrado pressionando ENTER na janela do processo.

## Branches

`feature/*` e `fix/*` seguem para `develop`; depois de validação suficiente, `develop` pode ser promovida para `main`. Branches `archive/*` são históricas.

## Regra de validação

Implementação, build, teste de bancada e validação física devem continuar documentados separadamente.
