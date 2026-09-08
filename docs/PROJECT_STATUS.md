# Estado do projeto

Atualização desta linha de desenvolvimento: 2026-09-08.

## Hardware de referência

`JRBOT-HW-04`

- ESP32-S3 N16R8: 16 MB flash + 8 MB OPI PSRAM
- OLED SSD1306: SDA GPIO1 / SCL GPIO2
- OV5640: câmera no conector da placa
- MAX98357A: BCLK GPIO21 / WS GPIO47 / DIN GPIO42
- MS3625: SCK GPIO21 / WS GPIO47 / SD GPIO41

Microfone e amplificador compartilham BCLK/WS e precisam arbitrar o recurso I2S.

## Consolidação do histórico

Antes da candidata 02, os heads existentes foram preservados em branches `archive/2026-09-08/*` e a cadeia acumulada foi integrada em `main`.

`develop` foi então alinhada ao novo `main`, e a candidata atual partiu dessa base consolidada.

## Firmware desta candidata

`JRBotV2_RUNTIME_API_V1_02`

Branch: `feature/runtime-api-v1-02`.

Objetivos isolados desta revisão:

1. corrigir o framing de saída do MAX98357A de MSB para I2S Philips;
2. evoluir a Runtime API de forma compatível, mantendo `v=1` e publicando API `1.1`;
3. adicionar a primeira action segura `face`.

## Runtime API — estado atual

### Validado fisicamente na candidata 01 pela Serial

Foram exercitados no hardware real:

- `capabilities`;
- `get("system.version")`;
- `get("audio.volume")`;
- `get("face.current")`;
- `get("brain.status")`;
- versão incompatível;
- função inexistente;
- path inexistente;
- JSON inválido;
- correlação de resposta por `id`;
- coexistência com comandos legados.

O teste HTTP local da candidata 01 não foi registrado como concluído antes da consolidação.

### Implementado no código da candidata 02

- envelope JSON com major `v=1`;
- API compatível reportada como `1.1`;
- `capabilities`;
- `get`;
- action `face`;
- erros estruturados;
- transporte `api <JSON>` pela Serial existente e por `POST /cmd`;
- leitura dos caminhos:
  - `api.version`;
  - `system.version`;
  - `system.hardware`;
  - `system.profile`;
  - `brain.status`;
  - `voice.status`;
  - `audio.volume`;
  - `face.current`;
  - `wifi.status`.

### Pendente de validação da candidata 02

- build ESP-IDF;
- gravação da candidata `JRBotV2_RUNTIME_API_V1_02`;
- `capabilities` anunciando API `1.1` e action `face`;
- `face("thinking")` alterando fisicamente o OLED;
- `get("face.current")` confirmando o mesmo estado;
- rejeição de expressão inválida;
- teste HTTP local;
- regressão básica da API candidata 01.

## Áudio MAX98357A

### Problema observado

O teste de áudio da candidata anterior transmitia dados com sucesso, mas o som físico foi relatado como muito ruim/ruído mesmo alterando o volume.

### Alteração da candidata 02

A configuração de saída foi alterada de `I2S_STD_MSB_SLOT_DEFAULT_CONFIG` para `I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG`, mantendo:

- 16 kHz;
- 16-bit;
- estéreo duplicado a partir do PCM mono;
- BCLK GPIO21;
- WS GPIO47;
- DIN GPIO42;
- gate binário de arbitragem I2S.

Essa mudança está **implementada no código, mas ainda não foi validada fisicamente**. O critério é o tom de `audio_test` ficar reconhecível e sem o ruído forte anterior.

Se o framing Philips não resolver, a investigação passa para alimentação, GND, SD/MODE, ligação física, alto-falante e eventual clipping de captura, sem atribuir automaticamente a falha ao volume.

## Microfone

O MS3625 foi validado gravando 3 segundos/48000 samples. Foi observado que o diagnóstico rápido pode mostrar `mic=unavailable` quando o probe não consegue adquirir o barramento I2S dentro do timeout curto; após uma gravação bem-sucedida o status voltou a `mic=available`.

Isso indica uma limitação semântica do diagnóstico atual entre `busy` e `unavailable`, não uma prova de falha física do microfone.

## Voz

O MultiNet6 continua sendo usado experimentalmente como detector contínuo do nome. Houve detecção acústica real de `JR BOT`, e o ciclo de resposta local já foi exercitado, mas falsos positivos continuam sendo problema aberto.

A candidata 02 não recalibra reconhecimento de voz.

## Planejado — ainda não implementado

- `set` persistente;
- Voice Registry dinâmico;
- Playground de calibração;
- confiança por frase configurável;
- `say`, `wait`, `listen`, `play` como actions da Runtime API;
- Flow Engine;
- eventos assíncronos;
- banco de comportamento/personality data;
- TTS geral para frases arbitrárias;
- integração VPS/IA avançada.

Consulte [API_RUNTIME.md](API_RUNTIME.md), [RUNTIME_API_V1_02_TEST.md](RUNTIME_API_V1_02_TEST.md), [PLAYGROUND.md](PLAYGROUND.md) e [FLOWS.md](FLOWS.md).

## Regra de evolução

Nova capacidade física ou sistêmica pode exigir firmware novo. Novo comportamento composto apenas por capabilities já disponíveis deve migrar para configuração/Flow em runtime, sem recompilar o firmware.
