# JrBot - reconhecimento do nome e resposta local

Branch: `feature/local-brain-jrbot-response`
Firmware esperado: `JRBotV2_JRBOT_RESPONSE_01`

## Objetivo

Validar o ciclo local completo:

```text
voz -> reconhecimento do nome -> Local Brain -> face feliz -> resposta sonora -> volta a escutar
```

## Reconhecimento principal

A candidata tenta carregar `mn6_en` (MultiNet6) e cadastra o mesmo comando com ID 1 usando os aliases:

- `JR BOT`
- `JUNIOR BOT`
- `J R BOT`

O painel apresenta o nome humano como `JrBot`.

Este uso do MultiNet como reconhecedor direto e continuo do nome e experimental. O fluxo recomendado pela Espressif e WakeNet seguido de MultiNet. Estamos usando este ensaio para avaliar se o nome curto funciona suficientemente bem no hardware real antes de obter um WakeNet personalizado.

## Fallback

Se o MultiNet nao carregar ou rejeitar todos os aliases, o firmware volta automaticamente ao WakeNet `wn9_hiesp` ja validado.

No log:

```text
engine=esp-sr-multinet model=mn6_en wakeword=JrBot
```

significa que o teste do nome esta ativo.

Se aparecer:

```text
engine=esp-sr-wakenet ... wakeword=Hi ESP
```

use `Hi ESP`, porque o firmware entrou no fallback.

## Resposta

Quando houver deteccao real:

1. Local Brain registra `wakeword_detected`;
2. OLED muda para `feliz`;
3. o microfone libera o barramento I2S;
4. MAX98357A toca dois tons curtos (880 Hz e 1320 Hz);
5. o firmware espera 180 ms para evitar eco/retrigger;
6. o microfone e reaberto;
7. o detector e limpo e volta a escutar.

O bip duplo e deliberadamente simples nesta etapa. Depois de validar o ciclo duplex, podemos substituir a resposta por uma frase PCM local como `Oi` ou por outro mecanismo de voz sem alterar a interface do Local Brain.

## Build

Abra o terminal ESP-IDF 5.5.x:

```bat
git fetch origin
git switch feature/local-brain-jrbot-response
git pull --ff-only origin feature/local-brain-jrbot-response
INSTALAR.bat build
```

Build isolado:

- `firmware/build-jrbot-response`
- `firmware/sdkconfig.jrbot-response`
- particao `model`: 8 MB

O build deve empacotar pelo menos `mn6_en`, `fst` e `wn9_hiesp`.

Depois de build sem erro:

```bat
INSTALAR.bat flash
PAINEL.bat
```

## Teste esperado

1. Conectar por Serial.
2. Confirmar `JRBotV2_JRBOT_RESPONSE_01`.
3. Ativar autonomo.
4. Conferir `brain_status`.
5. Se `engine=esp-sr-multinet`, falar `Jr Bot` e depois testar `Junior Bot`.
6. A face deve ficar feliz.
7. Deve tocar o bip duplo.
8. O log deve terminar com `listening_resumed=1`.

Exemplo esperado para MultiNet:

```text
jrbot_voice: MultiNet alias accepted: JR BOT
jrbot_voice: direct name recognizer ready model=mn6_en ...
JR_BRAIN event=autonomous_on engine=esp-sr-multinet model=mn6_en wakeword=JrBot ...
jrbot_voice: name detected keyword=JrBot ... model=mn6_en ...
JR_BRAIN event=wakeword_detected source=voice keyword=JrBot model=mn6_en trigger=1
JR_BRAIN event=reaction_happy expression=feliz
jrbot_voice: reply begin type=ack_chirp
jrbot_voice: reply end type=ack_chirp result=ESP_OK listening_resumed=1
```

## O que observar

Se o build falhar, enviar o trecho de `FAILED:` ate o final.

Se o MultiNet iniciar mas nao reconhecer o nome, salvar o log e testar separadamente `Jr Bot`, `Junior Bot` e `J R Bot`. Isso nos dira se o problema e tokenizacao/pronuncia ou se precisamos abandonar o atalho e partir diretamente para um WakeNet personalizado.

Se reconhecer e responder, o proximo passo recomendado e trocar o bip por uma resposta falada curta mantendo a pausa/resume do I2S que esta candidata valida.
