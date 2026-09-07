# JrBot - reconhecimento do nome e resposta local

Branch: `feature/local-brain-jrbot-response`
Firmware esperado: `JRBotV2_JRBOT_RESPONSE_03`

## Objetivo

Validar o ciclo local completo:

```text
voz -> "JrBot" -> Local Brain -> face feliz -> resposta "Oi" -> volta a escutar
```

## Reconhecimento

Esta candidata usa somente `mn6_en` (MultiNet6) e cadastra um unico comando com ID 1:

- `JR BOT`

Nao existem os aliases `JUNIOR BOT` ou `J R BOT` nesta versao.
Nao existe fallback `Hi ESP` nesta versao.

O painel apresenta o nome humano como `JrBot`.

O reconhecimento direto e continuo pelo MultiNet continua sendo experimental. O objetivo desta candidata e preservar exatamente o caminho que ja funcionou no teste fisico com `JR BOT`, sem ampliar o vocabulario.

## Filtro de confianca

A deteccao so e aceita quando:

- `command_id == 1`; e
- a probabilidade retornada pelo MultiNet e pelo menos `0.70`.

No teste fisico anterior, `JR BOT` foi reconhecido com probabilidade `0.751`, portanto esse caso permanece acima do corte.

Candidatos abaixo de `0.70` sao ignorados e registrados no log.

## Resposta falada

Quando houver deteccao valida:

1. Local Brain registra `wakeword_detected`;
2. OLED muda para `feliz`;
3. o microfone libera o barramento I2S;
4. o ESP32 gera localmente uma resposta robotica curta `Oi` em PCM 16 kHz;
5. MAX98357A reproduz a resposta;
6. o firmware espera 220 ms para reduzir eco/retrigger;
7. o microfone e reaberto;
8. o detector e limpo e volta a escutar.

A resposta e gerada no proprio ESP32, sem nuvem e sem depender do TTS do ESP-SR. Se a reproducao falhar, o firmware usa um bip curto como fallback de audio e tenta reabrir o microfone normalmente.

## Correcao do barramento I2S

O gate compartilhado do I2S agora usa semaforo binario, e nao mutex com dono. Isso permite o handoff entre a tarefa que ativa o modo autonomo e a tarefa `jr_voice`, evitando o assert `xTaskPriorityDisinherit` observado na candidata anterior.

## Build

Abra o terminal ESP-IDF 5.5.x:

```bat
git fetch origin
git switch feature/local-brain-jrbot-response
git pull --ff-only origin feature/local-brain-jrbot-response
INSTALAR.bat build
```

Build isolado desta candidata:

- `firmware/build-jrbot-response03`
- `firmware/sdkconfig.jrbot-response03`
- particao `model`: 8 MB

O novo build isolado evita reutilizar o sdkconfig que ainda continha WakeNet/aliases das candidatas anteriores.

Depois de build sem erro:

```bat
INSTALAR.bat flash
PAINEL.bat
```

## Teste esperado

1. Conectar por Serial.
2. Confirmar `JRBotV2_JRBOT_RESPONSE_03`.
3. Ativar autonomo.
4. Conferir `brain_status`.
5. Falar apenas `Jr Bot`.
6. A face deve ficar feliz.
7. O alto-falante deve responder `Oi`.
8. O log deve terminar com `listening_resumed=1`.
9. Falar outras frases comuns e verificar que elas nao geram resposta.

Exemplo esperado:

```text
jrbot_voice: MultiNet command accepted: JR BOT
jrbot_voice: JR BOT detection threshold=0.70 ...
jrbot_voice: direct JrBot recognizer ready model=mn6_en command=JR_BOT min_probability=0.70 ...
JR_BRAIN event=autonomous_on engine=esp-sr-multinet model=mn6_en wakeword=JrBot ...
jrbot_voice: name detected keyword=JrBot recognized="  JR BOT" model=mn6_en probability=... count=1
JR_BRAIN event=wakeword_detected source=voice keyword=JrBot model=mn6_en trigger=1
JR_BRAIN event=reaction_happy expression=feliz
jrbot_voice: reply begin type=spoken_oi
jrbot_voice: reply end type=spoken_oi result=ESP_OK listening_resumed=1
```

Um candidato fraco deve aparecer apenas como:

```text
JR BOT candidate ignored ... probability=... min=0.70
```

sem face feliz e sem resposta `Oi`.

## O que observar

Se o build falhar, enviar o trecho de `FAILED:` ate o final.

Se ainda houver falso positivo, salvar o log com a linha `name detected` ou `candidate ignored` e a probabilidade. A partir desses valores ajustamos o corte sem adicionar novos comandos.

Se reconhecer `JrBot`, responder `Oi` e voltar a escutar sem reboot, teremos validado o ciclo local completo com um unico nome: ouvir -> JrBot -> reagir -> falar -> voltar a ouvir.
