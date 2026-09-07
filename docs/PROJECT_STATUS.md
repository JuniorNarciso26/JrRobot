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

`JRBotV2_JRBOT_RESPONSE_05`

Objetivo desta revisão: recuperar o comportamento de reconhecimento da primeira candidata que detectou `JR BOT`, preservando a correção posterior do barramento I2S.

## Validado fisicamente

### Hardware e periféricos

- câmera OV5640 detectada e captura JPEG já exercitada;
- microfone MS3625 fornece amostras I2S;
- MAX98357A reproduz áudio no hardware;
- OLED responde no perfil de hardware atual.

### Voz

Foi registrada detecção acústica real pelo MultiNet6:

```text
recognized="  JR BOT"
probability=0.751
```

O ciclo atual também já executou repetidamente:

```text
detecção -> rosto feliz -> reprodução de "Oi" -> reabertura do microfone
```

sem reproduzir o antigo assert `xTaskPriorityDisinherit` após a troca do mutex por um gate binário para o I2S.

## Problema aberto principal

O MultiNet6 está sendo usado continuamente como detector experimental do nome. Ele é um reconhecedor de comandos e pode classificar fala/ruído como um dos comandos cadastrados.

Na candidata baseline com aliases `JR BOT`, `JUNIOR BOT` e `J R BOT`, foram observados falsos positivos frequentes, especialmente `J R BOT`, com probabilidades em diferentes faixas.

Portanto:

- captura de áudio: funcional;
- resposta e retomada do microfone: funcional;
- decisão “isso realmente foi JrBot?”: ainda em calibração.

## Planejado — ainda não implementado como contrato Runtime API

- Playground de calibração;
- confiança por frase configurável;
- persistência de frases e Flows;
- Runtime API v1 (`get`, `set`, `say`, `face`, `listen`, etc.);
- `capabilities()`;
- Flow Engine;
- banco de comportamento/personality data;
- TTS geral para frases arbitrárias;
- integração VPS/IA avançada.

As especificações estão em [API_RUNTIME.md](API_RUNTIME.md), [PLAYGROUND.md](PLAYGROUND.md) e [FLOWS.md](FLOWS.md).

## Regra de evolução

Nova capacidade física ou sistêmica pode exigir firmware novo. Novo comportamento composto apenas por capacidades já disponíveis deve, no futuro, ser criado como Flow/configuração sem recompilar o firmware.
