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

Os heads anteriores foram preservados em branches `archive/2026-09-08/*` antes da consolidação. A cadeia histórica de Local Brain, voz, resposta e Runtime API v1 candidata 01 foi incorporada em `main`; `develop` foi alinhada a essa base antes da candidata 02.

## Firmware validado nesta etapa

`JRBotV2_RUNTIME_API_V1_02`

Branch de origem: `feature/runtime-api-v1-02`.

Objetivos desta revisão:

1. corrigir o framing de saída do MAX98357A de MSB para I2S Philips;
2. evoluir a Runtime API de forma compatível, mantendo `v=1` e publicando API `1.1`;
3. adicionar a primeira action segura `face`.

## Runtime API — estado atual

### Implementado

- envelope JSON com major `v=1`;
- API compatível reportada como `1.1`;
- `capabilities`;
- `get`;
- action `face`;
- erros estruturados;
- transporte `api <JSON>` pela Serial e por `POST /cmd`;
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

### Validado fisicamente / em protocolo na candidata 02

- firmware `JRBotV2_RUNTIME_API_V1_02` executando na placa;
- `capabilities` anunciando API `1.1` e action `face`;
- `face("thinking")` pela Serial alterando fisicamente o OLED;
- `get("face.current")` confirmando `thinking`;
- expressão inválida rejeitada com `invalid_args / face_expression_not_supported`;
- regressão de `get("system.version")`, `get("audio.volume")` e `get("brain.status")`;
- `audio.volume` refletindo alteração real de 35 para 100;
- HTTP local `POST /cmd` para `get("system.version")`;
- HTTP local `POST /cmd` para `face("happy")`, com alteração física do OLED.

### Validação herdada da candidata 01, não repetida na 02

Na candidata 01 foram validados:

- versão incompatível -> `unsupported_version`;
- função inexistente;
- path inexistente;
- JSON inválido -> `invalid_json`;
- correlação de resposta por `id`;
- coexistência com comandos legados.

Nos logs finais da candidata 02, `unsupported_version` e `invalid_json` não foram repetidos. Essa lacuna permanece documentada e não é contada como nova validação física da revisão 02.

## Áudio MAX98357A

A saída foi alterada de `I2S_STD_MSB_SLOT_DEFAULT_CONFIG` para `I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG`, mantendo:

- 16 kHz;
- 16-bit;
- estéreo duplicado a partir do PCM mono;
- BCLK GPIO21;
- WS GPIO47;
- DIN GPIO42;
- gate binário de arbitragem I2S.

### Resultado físico

O `audio_test` registrou `format=PHILIPS` e `ESP_OK`. O ruído forte da candidata anterior desapareceu e o som foi avaliado como bom/limpo.

O volume acústico máximo continua baixo mesmo com volume digital em 100%. Nesta etapa isso foi aceito como limitação do alto-falante atual de 20 mm / 4 ohms / 3 W e da montagem acústica. A alimentação do amplificador foi medida em aproximadamente 4,99 V em repouso, com queda momentânea para 4,56 V e cerca de 4,88 V durante reprodução. Ligar `GAIN` ao GND não produziu diferença acústica relevante.

Melhoria do alto-falante/caixa acústica fica para uma etapa futura e não bloqueia a Runtime API.

## Microfone

O MS3625 foi validado gravando 3 segundos / 48000 samples. Foi observado que o diagnóstico rápido pode mostrar `mic=unavailable` quando o probe não consegue adquirir o barramento I2S dentro do timeout curto; após uma gravação bem-sucedida o status voltou a `mic=available`.

Isso indica uma limitação semântica do diagnóstico atual entre `busy` e `unavailable`, não uma prova de falha física do microfone.

## Voz

O MultiNet6 continua experimental como detector contínuo do nome. Houve detecção acústica real de `JR BOT` e o ciclo de resposta local já foi exercitado, mas falsos positivos continuam como problema aberto.

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