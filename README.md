# JrBot

JrBot é um robô experimental baseado em ESP32-S3, criado para evoluir de um controlador local de hardware para uma plataforma híbrida: capacidades essenciais offline no robô e recursos avançados de IA opcionais em serviços remotos.

## Estado atual

- Hardware: `JRBOT-HW-04`
- MCU: ESP32-S3 N16R8
- Candidata desta branch: `JRBotV2_RUNTIME_API_V1_02`
- Branch estável/promovida: `main`
- Branch de integração: `develop`
- Branch desta etapa: `feature/runtime-api-v1-02`
- Runtime API: major `v=1`, API compatível `1.1`
- Primeira action da API: `face`
- Áudio local: MAX98357A com candidata de framing I2S Philips
- Reconhecimento de voz: ESP-SR MultiNet6 experimental, preservado da etapa anterior
- Microfone: MS3625
- Câmera: OV5640
- Display: OLED SSD1306

> A candidata 02 está implementada no código, mas ainda precisa de build e validação física. O reconhecimento contínuo de `JrBot` continua experimental e não é recalibrado nesta revisão.

## Histórico consolidado

Antes desta candidata, os heads existentes foram preservados em branches `archive/2026-09-08/*`. A evolução acumulada de Local Brain, voz, resposta, documentação e Runtime API candidata 01 foi incorporada em `main`.

Depois da consolidação, `develop` foi alinhada ao novo `main` e a candidata 02 foi criada a partir dessa base.

## Estratégia de branches

```text
main
  └── develop
        ├── feature/runtime-api-v1-02
        ├── feature/...
        └── fix/...

archive/2026-09-08/*
  └── snapshots históricos dos heads anteriores à consolidação
```

- `main`: baselines promovidos/consolidados.
- `develop`: integração da próxima versão.
- `feature/*`: desenvolvimento isolado criado a partir de `develop` e integrado de volta por Pull Request.
- `fix/*`: correções isoladas seguindo a mesma regra de revisão.
- `archive/*`: referências históricas que não devem receber desenvolvimento novo.

## Runtime API v1 desta candidata

Implementado no código:

```text
capabilities()
get(path)
face(expression)
```

Exemplo de action:

```text
api {"v":1,"id":"face01","fn":"face","args":{"expression":"thinking"}}
```

Depois é possível conferir o mesmo estado real:

```text
api {"v":1,"id":"face02","fn":"get","args":{"path":"face.current"}}
```

A API não executa código textual arbitrário nem aceita GPIO direto. A action `face` chama somente a capability segura registrada no firmware.

`set`, Playground, Flow Engine, persistência, `say`, `listen` e outras actions continuam fora desta candidata.

## Áudio da candidata 02

O MAX98357A estava transmitindo dados com `ESP_OK`, mas o teste físico foi relatado como ruído/som muito ruim em diferentes volumes.

Nesta candidata o framing de saída foi alterado de MSB para I2S Philips, mantendo o gate binário do barramento I2S e a pinagem existente.

Isso é uma **hipótese de correção implementada**, não uma correção fisicamente validada ainda. O primeiro teste deve ser `audio_test` em volume baixo/moderado e confirmação no log de `format=PHILIPS`.

## Comece pela documentação

A documentação oficial fica em [`docs/`](docs/README.md). O painel local possui uma área **Documentação** que lê esses mesmos arquivos Markdown.

Principais documentos:

- [Mapa da documentação](docs/README.md)
- [Estado do projeto](docs/PROJECT_STATUS.md)
- [Arquitetura](docs/ARCHITECTURE.md)
- [Guia de desenvolvimento](docs/DEVELOPMENT.md)
- [Runtime API v1](docs/API_RUNTIME.md)
- [Teste da candidata 02](docs/RUNTIME_API_V1_02_TEST.md)
- [Playground](docs/PLAYGROUND.md)
- [Flows](docs/FLOWS.md)
- [Testes e validação](docs/TESTING.md)

## Compilar e gravar a candidata

No terminal ESP-IDF 5.5.x:

```bat
git fetch --all --prune
git switch feature/runtime-api-v1-02
git pull --ff-only origin feature/runtime-api-v1-02
INSTALAR.bat build
INSTALAR.bat flash
PAINEL.bat
```

Confirme no painel que a placa reporta:

```text
JRBotV2_RUNTIME_API_V1_02
```

antes de interpretar qualquer teste.

## Filosofia técnica

1. Firmware fornece capacidades seguras; configurações definem comportamento.
2. IA nunca controla GPIO diretamente.
3. Mudanças de comportamento devem migrar para dados/Flows em runtime sempre que possível.
4. Teste em computador, build e validação física são estados diferentes e devem ser documentados separadamente.
5. O robô precisa manter identidade e funções básicas mesmo sem VPS ou internet.

## Contribuição

Leia [`CONTRIBUTING.md`](CONTRIBUTING.md) antes de abrir mudanças.

## Licença

O repositório ainda não possui um arquivo `LICENSE`. Uma licença open source deve ser escolhida explicitamente pelo mantenedor antes de tratar o código como redistribuível sob uma licença específica.
