# JrBot

JrBot é um robô experimental baseado em ESP32-S3, criado para evoluir de um controlador local de hardware para uma plataforma híbrida: capacidades essenciais offline no robô e recursos avançados de IA opcionais em serviços remotos.

## Estado atual

- Hardware: `JRBOT-HW-04`
- MCU: ESP32-S3 N16R8
- Baseline promovida: `JRBotV2_RUNTIME_API_V1_02`
- Branch estável: `main`
- Branch de integração: `develop`
- Runtime API: major `v=1`, API compatível `1.1`
- Funções da API: `capabilities`, `get`, `face`
- Primeira action física validada: `face`
- Transporte validado: Serial e HTTP local `POST /cmd`
- Áudio local: MAX98357A com framing I2S Philips validado fisicamente sem o ruído forte anterior
- Reconhecimento de voz: ESP-SR MultiNet6 experimental
- Microfone: MS3625
- Câmera: OV5640
- Display: OLED SSD1306

A candidata `JRBotV2_RUNTIME_API_V1_02` foi exercitada no hardware real. O som ficou limpo após a troca para I2S Philips; o volume acústico máximo continua baixo com o alto-falante atual de 20 mm / 4 ohms / 3 W e essa limitação foi aceita para esta baseline.

O reconhecimento contínuo de `JrBot` continua experimental e não foi recalibrado nesta revisão.

## Histórico consolidado

Os heads anteriores foram preservados em branches `archive/2026-09-08/*` antes das integrações. A evolução acumulada de Local Brain, voz, resposta, documentação, Runtime API candidata 01 e Runtime API candidata 02 foi incorporada pela cadeia de Pull Requests `feature -> develop -> main`.

As branches `archive/*` são referências históricas e não devem receber desenvolvimento novo.

## Estratégia de branches

```text
main
  └── develop
        ├── feature/...
        └── fix/...

archive/2026-09-08/*
  └── snapshots históricos
```

- `main`: baselines promovidas/consolidadas.
- `develop`: integração da próxima versão.
- `feature/*`: desenvolvimento isolado criado a partir de `develop` e integrado de volta por Pull Request.
- `fix/*`: correções isoladas seguindo a mesma regra de revisão.
- `archive/*`: referências históricas.

## Runtime API v1.1

Disponível:

```text
capabilities()
get(path)
face(expression)
```

Exemplo pela Serial ou pelo corpo de `POST /cmd`:

```text
api {"v":1,"id":"face01","fn":"face","args":{"expression":"thinking"}}
```

Depois é possível conferir o estado real:

```text
api {"v":1,"id":"face02","fn":"get","args":{"path":"face.current"}}
```

A API não executa código textual arbitrário nem aceita GPIO direto. `face` chama somente a capability segura registrada no firmware.

Também foi validado pela rede local:

```text
PC -> Wi-Fi -> HTTP /cmd -> Runtime API -> face() -> OLED físico
```

O portal HTTP local não possui autenticação/TLS e não deve ser exposto diretamente à Internet.

`set`, Playground, Flow Engine, persistência, `say`, `listen` e outras actions continuam fora desta baseline.

## Áudio

O MAX98357A usa:

```text
BCLK GPIO21
WS   GPIO47
DIN  GPIO42
16 kHz / 16-bit
I2S Philips
```

A troca de MSB para Philips eliminou o ruído forte observado anteriormente. O controle digital de volume continua funcional, mas a saída acústica máxima é limitada pelo alto-falante atual e pela montagem física.

## Validação e ressalvas

Validado nesta baseline:

- firmware `JRBotV2_RUNTIME_API_V1_02` executando na placa;
- `capabilities` API `1.1`;
- `get` de estados principais;
- `face("thinking")` pela Serial com mudança física do OLED;
- `get("face.current")` confirmando o estado;
- rejeição de expressão inválida;
- HTTP local para `get("system.version")`;
- HTTP local para `face("happy")` com mudança física do OLED;
- I2S Philips com áudio limpo.

Ressalvas registradas:

- o log completo da etapa de compilação ESP-IDF não foi anexado ao registro final, embora a candidata tenha sido gravada e executada fisicamente;
- `unsupported_version` e `invalid_json` foram validados na candidata 01, mas não repetidos especificamente nos logs finais da candidata 02;
- o diagnóstico rápido do microfone ainda pode confundir barramento I2S ocupado com `mic=unavailable`;
- falsos positivos de reconhecimento contínuo de voz continuam em aberto;
- volume acústico do alto-falante atual é baixo.

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

## Compilar e gravar a baseline atual

No terminal ESP-IDF 5.5.x:

```bat
git fetch --all --prune
git switch main
git pull --ff-only origin main
INSTALAR.bat build
INSTALAR.bat flash
PAINEL.bat
```

Confirme no painel:

```text
JRBotV2_RUNTIME_API_V1_02
```

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
