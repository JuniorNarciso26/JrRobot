# JrBot

JrBot é um robô experimental baseado em ESP32-S3, criado para evoluir de um controlador local de hardware para uma plataforma híbrida: capacidades essenciais offline no robô e recursos avançados de IA opcionais em serviços remotos.

## Estado atual

- Hardware: `JRBOT-HW-04`
- MCU: ESP32-S3 N16R8
- Candidata desta branch: `JRBotV2_RUNTIME_API_V1_01`
- Branch estável/promovida: `main`
- Branch de integração: `develop`
- Branch desta etapa: `feature/runtime-api-v1`
- Runtime API: base v1 implementada no código; build e validação ainda pendentes
- Reconhecimento de voz: ESP-SR MultiNet6 experimental, preservado da etapa anterior
- Áudio local: MAX98357A
- Microfone: MS3625
- Câmera: OV5640
- Display: OLED SSD1306

> O reconhecimento contínuo de `JrBot` por MultiNet é experimental. A Runtime API desta candidata não altera a calibração de voz. Recursos descritos como planejados na documentação não devem ser tratados como implementados.

## Estratégia de branches

```text
main
  └── develop
        ├── feature/runtime-api-v1
        ├── feature/...
        └── fix/...
```

- `main`: somente baselines promovidos após validação adequada.
- `develop`: integração da próxima versão.
- `feature/*`: desenvolvimento isolado criado a partir de `develop` e integrado de volta por Pull Request.
- `fix/*`: correções isoladas, seguindo a mesma regra de revisão.

As branches `feature/local-brain-autonomous`, `feature/local-brain-voice` e `feature/local-brain-jrbot-response` registram a evolução histórica que originou o estado de base de `develop`.

## Runtime API v1 desta candidata

Implementado no código:

```text
capabilities()
get(path)
```

Transporte inicial:

```text
api {"v":1,"fn":"capabilities","args":{}}
```

A resposta usa o prefixo `JR_API` e JSON estruturado. `set`, Playground, Flow Engine, persistência e actions ainda não fazem parte desta candidata.

## Comece pela documentação

A documentação oficial fica em [`docs/`](docs/README.md). O painel local também possui uma área **Documentação** que lê esses mesmos arquivos Markdown, sem criar uma segunda fonte de verdade.

Principais documentos:

- [Mapa da documentação](docs/README.md)
- [Estado do projeto](docs/PROJECT_STATUS.md)
- [Arquitetura](docs/ARCHITECTURE.md)
- [Guia de desenvolvimento](docs/DEVELOPMENT.md)
- [Runtime API v1](docs/API_RUNTIME.md)
- [Roteiro de teste Runtime API v1](docs/RUNTIME_API_V1_TEST.md)
- [Playground](docs/PLAYGROUND.md)
- [Flows](docs/FLOWS.md)
- [Testes e validação](docs/TESTING.md)

## Compilar e gravar a candidata

No terminal ESP-IDF 5.5.x:

```bat
git switch feature/runtime-api-v1
git pull --ff-only origin feature/runtime-api-v1
INSTALAR.bat build
INSTALAR.bat flash
PAINEL.bat
```

Confirme no painel que a placa reporta `JRBotV2_RUNTIME_API_V1_01` antes de interpretar qualquer teste.

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
