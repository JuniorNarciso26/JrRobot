# Estado do projeto

Atualização desta linha de desenvolvimento: 2026-09-07.

## Hardware de referência

`JRBOT-HW-04`

- ESP32-S3 N16R8: 16 MB flash + 8 MB OPI PSRAM
- OLED SSD1306: SDA GPIO1 / SCL GPIO2
- OV5640: câmera no conector da placa
- MAX98357A: BCLK GPIO21 / WS GPIO47 / DIN GPIO42
- MS3625: SCK GPIO21 / WS GPIO47 / SD GPIO41

Microfone e amplificador compartilham BCLK/WS e precisam arbitrar o recurso I2S.

## Firmware desta candidata

`JRBotV2_RUNTIME_API_V1_01`

Branch: `feature/runtime-api-v1`.

Objetivo: criar a fundação versionada da Runtime API sem alterar a calibração de voz nesta etapa.

## Runtime API v1 — estado atual

### Implementado no código

- envelope JSON `v=1`;
- `capabilities`;
- `get`;
- erros estruturados;
- comando de transporte `api <JSON>`;
- uso via Serial e via `POST /cmd` do portal local;
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

### Ainda não validado nesta candidata

O código da Runtime API foi adicionado ao repositório, mas esta revisão ainda precisa passar por:

1. build ESP-IDF;
2. gravação na placa;
3. teste Serial de `capabilities`;
4. teste Serial de `get`;
5. teste HTTP local;
6. verificação de regressão dos periféricos existentes.

Até esses testes ocorrerem, a Runtime API deve ser descrita como **implementada no código**, não como fisicamente validada.

## Validado fisicamente em etapas anteriores

### Hardware e periféricos

- câmera OV5640 detectada e captura JPEG exercitada;
- microfone MS3625 fornece amostras I2S;
- MAX98357A reproduz áudio no hardware;
- OLED responde no perfil de hardware atual.

### Voz

Foi registrada detecção acústica real pelo MultiNet6:

```text
recognized="  JR BOT"
probability=0.751
```

Também já foi exercitado o ciclo:

```text
detecção -> rosto feliz -> reprodução de "Oi" -> reabertura do microfone
```

sem o antigo assert `xTaskPriorityDisinherit` após a troca do mutex por gate binário no barramento I2S.

## Problema aberto de voz

O MultiNet6 continua sendo usado experimentalmente como detector contínuo do nome. A candidata anterior apresentou falsos positivos, especialmente pelo alias `J R BOT`.

A Runtime API v1 não modifica essa lógica nesta primeira candidata. Isso é intencional para que eventuais regressões de API possam ser separadas dos problemas de calibração do reconhecimento.

## Planejado — ainda não implementado

- `set` na Runtime API;
- persistência de configuração;
- Voice Registry dinâmico;
- Playground de calibração;
- confiança por frase configurável;
- `say`, `face`, `wait`, `listen`, `play` como actions registradas;
- Flow Engine;
- eventos assíncronos;
- banco de comportamento/personality data;
- TTS geral para frases arbitrárias;
- integração VPS/IA avançada.

Consulte [API_RUNTIME.md](API_RUNTIME.md), [PLAYGROUND.md](PLAYGROUND.md) e [FLOWS.md](FLOWS.md).

## Regra de evolução

Nova capacidade física ou sistêmica pode exigir firmware novo. Novo comportamento composto apenas por capabilities já disponíveis deve migrar para configuração/Flow em runtime, sem recompilar o firmware.
