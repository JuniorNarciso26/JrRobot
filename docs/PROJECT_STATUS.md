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

A candidata atual é `JrBot_V1.4`.

Já estão implementados no GitHub para esta candidata:

- painel local responsivo para celular;
- controle de expressões e volume;
- gravação pelo microfone do próprio JrBot, reprodução no navegador, download e reprodução no robô;
- interface de câmera sem placeholder inicial, com captura sob demanda e opção de salvar a foto;
- gravação/seleção de uma mensagem no celular ou computador;
- conversão no navegador para WAV PCM 16-bit, mono, 16 kHz, até 10 segundos;
- envio da mensagem por `POST /audio` e reprodução no JrBot;
- manutenção do fluxo de microfone do JrBot sem substituí-lo pelo novo fluxo do celular.

Essas mudanças estão implementadas, mas a candidata `JrBot_V1.4` ainda precisa passar por build, flash e validação física no ESP32-S3 e no celular.

O `PAINEL.bat` continua sendo ferramenta de instalação, desenvolvimento e diagnóstico. O painel servido pelo ESP32 é a interface de uso da V1.

## Nomenclatura

- JrBot V1, V2 e V3: versões do produto.
- Runtime API 1.x: versão do protocolo.
- Firmware da linha V1: `JrBot_V1.4`, `JrBot_V1.5`, etc.

As revisões de firmware avançam dentro da branch `v1`; não é criada uma branch nova para cada revisão.

## Baseline técnica promovida

A baseline promovida em `main` continua sendo `JRBotV2_RUNTIME_API_V1_02`, com Runtime API `1.1`.

Os testes já registrados incluem mudança de face, leitura de estados, HTTP local, microfone e áudio I2S Philips.

O reconhecimento de voz MultiNet6 continua experimental e pertence à frente da JrBot V2.

## Instalador

Na organização atual, `INSTALAR.bat` trabalha com os canais oficiais `main`, `develop`, `v1` e `v2`, sincroniza o projeto com o GitHub, compila, grava o ESP32 e abre o painel de desenvolvimento.

O painel de desenvolvimento pode ser encerrado pressionando ENTER na janela do processo.

## Branches

- `main`: última versão aprovada;
- `develop`: integração;
- `v1`: JrBot V1 em desenvolvimento;
- `v2`: JrBot V2 preservada/pausada;
- `archive/*`: histórico.

## Regra de validação

Implementação, build, teste de bancada e validação física devem continuar documentados separadamente.
