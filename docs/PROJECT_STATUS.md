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

A etapa ativa é a JrBot V1, na branch `v1`, com PR #20 para `develop`.

A candidata atual é `JrBot_V1.4.1`.

O painel interno básico já foi exercitado no hardware real. A candidata atual acrescenta a interface de câmera e o fluxo de áudio do celular para o JrBot, preservando integralmente a gravação pelo microfone do próprio JrBot.

Correção `1.4.1`: o botão `Gravar mensagem` não usa mais `input capture`, que no Safari/iPhone pode abrir a câmera de vídeo. A gravação direta passa a solicitar somente áudio via `getUserMedia({audio:true, video:false})` quando o navegador permite. Em conexões HTTP locais onde o navegador bloqueia acesso ao microfone, a interface não abre a câmera e orienta usar `Escolher áudio` como alternativa.

O `PAINEL.bat` continua sendo ferramenta de instalação, desenvolvimento e diagnóstico. O painel servido pelo ESP32 é a interface de uso da V1.

## Nomenclatura

- JrBot V1, V2 e V3: versões do produto.
- Runtime API 1.x: versão do protocolo.
- Revisões de firmware da V1 seguem o padrão `JrBot_V1.x` e correções incrementais usam `JrBot_V1.x.y`.

Exemplo: `JrBot_V1.4` e a correção `JrBot_V1.4.1`.

## Baseline técnica promovida

A baseline promovida em `main` continua sendo `JRBotV2_RUNTIME_API_V1_02`, com Runtime API `1.1`.

Os testes já registrados incluem mudança de face, leitura de estados, HTTP local, microfone e áudio I2S Philips.

O reconhecimento de voz MultiNet6 continua experimental e pertence à frente da JrBot V2.

## Instalador

Na linha atual, `INSTALAR.bat` consulta o GitHub, permite escolher os canais oficiais `main`, `develop`, `v1` e `v2`, sincroniza o projeto, compila, grava o ESP32 e abre o painel de desenvolvimento.

O painel de desenvolvimento pode ser encerrado pressionando ENTER na janela do processo.

## Branches

- `main`: última versão aprovada.
- `develop`: integração.
- `v1`: JrBot V1 em desenvolvimento.
- `v2`: JrBot V2 voz.
- `archive/*`: histórico.

## Regra de validação

Implementação, build, teste de bancada e validação física devem continuar documentados separadamente.
