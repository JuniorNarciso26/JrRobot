# Arquitetura inicial do JB

## Base registrada

- Repositório upstream: `https://github.com/78/xiaozhi-esp32`
- Commit fixado: `dd99da00dc4c89ed4ab07fcec038c03f13f4de50`
- Submódulo local: `jb-firmware/`
- Branch local de trabalho: `feat/jb-hybrid-local-router`
- SDK indicado pela base: ESP-IDF 6.0.2 (a README também aceita versões estáveis 6.0 ou posteriores).

A base está em submódulo para preservar sua origem, histórico e licença. Nenhuma alteração foi feita no repositório upstream.

## Fluxo observado na base Xiaozhi

```text
WakeNet / MultiNet ou botão
  -> AudioService
  -> Application::HandleWakeWordDetectedEvent / StartListening
  -> Protocol::OpenAudioChannel
  -> Application::StartListeningAudio
  -> Protocol::SendStartListening
  -> AfeAudioEngine codifica áudio
  -> MAIN_EVENT_SEND_AUDIO
  -> Protocol::SendAudio
  -> servidor faz STT / regras / LLM
  -> JSON `stt` e/ou `tts` retorna ao firmware
  -> AudioService decodifica e reproduz o áudio recebido
```

Referências auditadas:

- `main/audio/audio_service.cc`: o callback `OnOutput` coloca PCM codificado na fila de envio; o callback de wake word sinaliza a aplicação.
- `main/application.cc`: `StartListeningAudio()` envia `listen:start` antes de habilitar o processamento de voz; `MAIN_EVENT_SEND_AUDIO` chama `Protocol::SendAudio`.
- `main/application.cc`: mensagens JSON `stt` chegam somente do servidor, depois da transmissão do áudio.
- `main/protocols/protocol.cc`: define `listen:start`, `listen:stop` e a transmissão de wake word.
- `main/audio/README.md` e `main/audio/engines/afe_audio_engine.cc`: no ESP32-S3 a AFE oferece WakeNet, VAD e caminhos MultiNet.

## Decisão de integração

O `LocalIntentRouter` será um módulo independente em `jb-firmware/main/jb/` e não chamará rede nem LLM.

Há três entradas distintas:

| Entrada | Onde entra | Decisão inicial |
|---|---|---|
| Evento local (hora, presença, dono reconhecido) | ponte JB chamada pela aplicação | pode resolver e responder localmente antes de abrir canal |
| ID de comando off-line | adaptador futuro do motor compatível | pode resolver localmente, se o modelo/língua forem confirmados |
| Texto STT | recebido em `Application::InitializeProtocol` | chega tarde demais para impedir o primeiro upload de áudio na arquitetura atual |

O ponto de integração de texto será o manipulador da mensagem JSON `type: "stt"`, para classificar e registrar a rota. Isso **não** impede o áudio já enviado ao servidor. Portanto, para cumprir a regra “LLM somente quando UNKNOWN”, será necessário um contrato de servidor que aplique o mesmo router/regras após o STT e antes da LLM. Não será alegado STT local em português no ESP32.

O primeiro corte do firmware criará a interface e testes host-side do router. A alteração no fluxo de `Application` só será feita depois de definir o contrato servidor-firmware e uma resposta local que a base consiga reproduzir.

## Rotas previstas

```text
Local event/offline command -> LocalIntentRouter -> LOCAL
STT do servidor -> regras do servidor/JB -> RULE_SERVER
UNKNOWN confirmado e on-line -> LLM_FALLBACK
UNKNOWN sem rede -> resposta curta local; sem nova tentativa automática
```

Uma intenção conhecida nunca terá dependência direta de cliente LLM. Logs futuros não registrarão áudio bruto, nome do usuário, nem texto pessoal completo.

## Português e áudio

- Não há confirmação de MultiNet off-line para pt-BR; ele não será usado como reconhecimento de frases em português nesta fase.
- Wake word temporária deve usar somente um modelo oficialmente compatível até que `Ei, JB` seja treinado e validado.
- Não será prometido TTS pt-BR off-line. As primeiras respostas terão texto e um stub de reprodução/log; áudio pré-gravado só será adotado após confirmar o formato suportado pela base.

## Armazenamento de pacotes

A primeira versão usará a partição de assets da Flash interna para o pacote `default/pt-BR`, pois a base já constrói e grava `assets.bin` em placas compatíveis e a placa não teve microSD auditado. O pacote será pequeno e somente leitura neste corte.

MicroSD fica adiado: faltam esquema elétrico, GPIOs expostos e definição de interface. Não haverá código ou pinagem de microSD até essa confirmação.

## GPIOs da placa JB — estado atual

| Função | GPIO | Estado |
|---|---:|---|
| Câmara XCLK | 15 | ocupado, não reutilizar |
| Câmara SIOD | 4 | ocupado, não reutilizar |
| Câmara SIOC | 5 | ocupado, não reutilizar |
| Câmara D0..D7 | 11, 9, 8, 10, 12, 18, 17, 16 | ocupados, não reutilizar |
| Câmara VSYNC / HREF / PCLK | 6 / 7 / 13 | ocupados, não reutilizar |
| Microfone I2S | — | bloqueado: GPIOs expostos e esquema não confirmados |
| Amplificador I2S | — | bloqueado: GPIOs expostos e esquema não confirmados |
| Tela / microSD / botões | — | bloqueado: GPIOs expostos, USB/JTAG e periféricos não confirmados |

Nenhum GPIO novo foi atribuído. USB/JTAG e pinos de strapping permanecerão sem uso até a auditoria do esquema.

## Dependências e licenças

| Dependência efetivamente usada | Uso | Licença / cuidado |
|---|---|---|
| Xiaozhi ESP32 | base de firmware | MIT; `jb-firmware/LICENSE` foi preservado |
| ESP-IDF | SDK de compilação | versão será confirmada no ambiente de build; avisos próprios devem ser preservados |
| `espressif/esp-sr` `~2.4.7` | dependência declarada pela base para AFE/WakeNet/MultiNet | validar a licença do componente resolvido antes de redistribuir firmware |

ESP-Skainet, microWakeWord e Speech-to-Intent-Micro são somente referências nesta etapa: nenhum código, modelo ou dependência deles foi adicionado. Antes de usar qualquer um, sua README, licença e compatibilidade com a placa serão revisadas e registradas.

## Próxima etapa verificável

Criar o núcleo C++ puro do `LocalIntentRouter` com normalização pt-BR, regras conservadoras, `kUnknown`, motivo/confiança e testes host-side. Não requer hardware, rede, áudio, pinagem ou LLM.
