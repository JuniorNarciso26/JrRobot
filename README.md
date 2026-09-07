# JrBot

JrBot é um robô experimental baseado em ESP32-S3, criado para evoluir de um controlador local de hardware para uma plataforma híbrida: capacidades essenciais offline no robô e recursos avançados de IA opcionais em serviços remotos.

## Estado atual

- Hardware: `JRBOT-HW-04`
- MCU: ESP32-S3 N16R8
- Firmware desta linha de desenvolvimento: `JRBotV2_JRBOT_RESPONSE_05`
- Branch de desenvolvimento desta etapa: `feature/local-brain-jrbot-response`
- Reconhecimento de voz atual: ESP-SR MultiNet6 experimental
- Áudio local: MAX98357A
- Microfone: MS3625
- Câmera: OV5640
- Display: OLED SSD1306

> O reconhecimento contínuo de `JrBot` por MultiNet é experimental. Há detecção acústica real validada, mas a calibração de falsos positivos ainda está em desenvolvimento. Recursos descritos como planejados na documentação não devem ser tratados como implementados.

## Comece pela documentação

A documentação oficial fica em [`docs/`](docs/README.md). O painel local também possui uma área **Documentação** que lê esses mesmos arquivos Markdown, sem criar uma segunda fonte de verdade.

Principais documentos:

- [Mapa da documentação](docs/README.md)
- [Estado do projeto](docs/PROJECT_STATUS.md)
- [Arquitetura](docs/ARCHITECTURE.md)
- [Guia de desenvolvimento](docs/DEVELOPMENT.md)
- [Runtime API v1](docs/API_RUNTIME.md)
- [Playground](docs/PLAYGROUND.md)
- [Flows](docs/FLOWS.md)
- [Testes e validação](docs/TESTING.md)

## Compilar e gravar a candidata atual

No terminal ESP-IDF 5.5.x:

```bat
git switch feature/local-brain-jrbot-response
git pull --ff-only origin feature/local-brain-jrbot-response
INSTALAR.bat build
INSTALAR.bat flash
PAINEL.bat
```

Confirme no painel que a placa reporta a versão esperada antes de interpretar qualquer teste.

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
