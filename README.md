# JrBot

JrBot é um robô experimental baseado em ESP32-S3, criado para evoluir de um controlador local de hardware para uma plataforma híbrida com capacidades locais e recursos avançados de IA opcionais.

## Estado atual

- Hardware: `JRBOT-HW-04`
- MCU: ESP32-S3 N16R8
- Baseline promovida: `JRBotV2_RUNTIME_API_V1_02`
- Branch estável: `main`
- Branch de integração: `develop`
- Trabalho ativo da JrBot V1: `feature/v1-internal-web-panel`
- Runtime API promovida: major `v=1`, API compatível `1.1`
- Áudio: MAX98357A
- Microfone: MS3625
- Câmera: OV5640
- Display: OLED SSD1306

A baseline técnica promovida continua sendo `JRBotV2_RUNTIME_API_V1_02`. O roadmap do produto, porém, passa a usar JrBot V1, V2 e V3. Essas versões não são equivalentes às versões da Runtime API ou às candidatas de firmware.

## Roadmap do produto

```text
JrBot V1  -> painel interno pelo IP
JrBot V2  -> comandos por voz
JrBot V3  -> controle programático por API
JrBrain   -> memória, personalidade, LLM e comportamento
```

### JrBot V1

Interface homem-robô servida pelo próprio ESP32-S3 na rede local. O objetivo é controlar rosto, áudio, microfone e câmera pelo navegador, com experiência adequada para celular.

O `PAINEL.bat` e o painel Python continuam sendo ferramentas de instalação, desenvolvimento e diagnóstico. O painel interno do ESP32 é a interface normal de uso da V1.

### JrBot V2

Controle das mesmas capabilities por voz, por exemplo:

```text
JrBot feliz
JrBot tocar som abc.wav
```

Playground, calibração, threshold e latência pertencem a esta frente.

### JrBot V3

Controle programático das mesmas capabilities por software externo. A V3 é definida pelo tipo de integração, não por um transporte específico; HTTP/IP e Serial podem continuar sendo utilizados.

### JrBrain

Depois da fundação V1-V3, o projeto avança para memória, identidade, personalidade, contexto, LLM, skills e comportamento composto por capabilities.

Leia o detalhamento em [`docs/ROADMAP.md`](docs/ROADMAP.md).

## Regra de nomenclatura

```text
JrBot V1 / V2 / V3
= versão do produto

Runtime API 1.1 / 1.2 / 1.3
= versão do protocolo/capabilities

JRBot..._V1_02 / _V1_03 / _V1_04
= candidata ou revisão técnica de firmware
```

## Estratégia de branches

```text
main
  └── develop
        ├── feature/...
        └── fix/...

archive/*
  └── snapshots históricos
```

- `main`: baselines promovidas.
- `develop`: integração da próxima versão.
- `feature/*`: desenvolvimento isolado criado a partir de `develop`.
- `fix/*`: correções isoladas.
- `archive/*`: referências históricas, sem desenvolvimento novo.

## Runtime API promovida

A baseline promovida oferece `capabilities`, `get` e `face`, com transporte Serial e HTTP local já exercitado em hardware.

O portal HTTP local é destinado à rede local confiável.

## Instalação e atualização

Na linha atual, `INSTALAR.bat` consulta o GitHub, atualiza a lista de branches ativas, permite escolher a versão/branch, sincroniza o projeto, compila, grava o ESP32 e abre o painel de desenvolvimento. Branches `archive/*` não aparecem como canais ativos.

## Documentação

A documentação oficial fica em [`docs/`](docs/README.md).

Principais documentos:

- [Roadmap oficial](docs/ROADMAP.md)
- [Estado do projeto](docs/PROJECT_STATUS.md)
- [Arquitetura](docs/ARCHITECTURE.md)
- [Guia de desenvolvimento](docs/DEVELOPMENT.md)
- [Runtime API](docs/API_RUNTIME.md)
- [Playground](docs/PLAYGROUND.md)
- [Flows](docs/FLOWS.md)
- [Testes e validação](docs/TESTING.md)

## Filosofia técnica

1. Firmware fornece capacidades seguras; configurações definem comportamento.
2. IA não controla GPIO diretamente.
3. Teste em computador, build e validação física são estados diferentes e devem ser documentados separadamente.
4. O robô precisa manter funções básicas mesmo sem VPS ou internet.

## Contribuição

Leia [`CONTRIBUTING.md`](CONTRIBUTING.md) antes de abrir mudanças.

## Licença

O repositório ainda não possui um arquivo `LICENSE`.
