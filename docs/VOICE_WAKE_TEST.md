# JrBot - teste de reconhecimento de voz local

Branch: `feature/local-brain-voice`
Firmware esperado: `JRBotV2_VOICE_01`

## Objetivo

Substituir o `bench_stub` do primeiro Local Brain por reconhecimento de wake word real no ESP32-S3, sem alterar a cadeia de reacao que ja foi validada.

Nesta candidata:

- microfone: MS3625 / I2S;
- engine: ESP-SR 2.5.1 / WakeNet;
- modelo de teste: `wn9_hiesp`;
- palavra falada real: **Hi ESP**;
- resposta: OLED muda para `feliz`;
- depois da reacao o robo continua em escuta.

O botao de simulacao continua existindo como diagnostico independente do reconhecimento.

## Por que ainda nao e "JrBot"

O modelo publico personalizado `JrBot` ainda nao existe. Esta branch usa um modelo oficial para validar o caminho fisico completo:

```text
MS3625 -> I2S -> WakeNet -> wakeword_detected -> Local Brain -> face feliz
```

Quando o modelo personalizado `JrBot` estiver disponivel, a arquitetura nao muda: substituimos o modelo selecionado na particao `model`.

## Instalacao

No terminal ESP-IDF 5.5.x:

```bat
git fetch origin
git switch feature/local-brain-voice
git pull --ff-only origin feature/local-brain-voice
INSTALAR.bat build
```

Esta branch usa build isolado:

- build: `firmware/build-localbrain-voice`
- sdkconfig: `firmware/sdkconfig.localbrain-voice`
- particoes: `firmware/partitions_sr.csv`

Isso evita reutilizar a tabela de particoes da branch anterior.

Depois de um build sem erros:

```bat
INSTALAR.bat flash
PAINEL.bat
```

O `flash` grava tanto o firmware quanto a particao de modelos ESP-SR.

## Teste

1. Conectar o painel pela Serial.
2. Confirmar firmware `JRBotV2_VOICE_01`.
3. Clicar **Ativar autonomo**.
4. Esperar o log informar `engine=esp-sr-wakenet` e `listening`.
5. Falar **Hi ESP** perto do microfone.
6. Confirmar `wakeword_detected source=voice` no log.
7. Confirmar OLED em `feliz`.
8. Confirmar que aparece novamente `event=listening`.

Exemplo esperado:

```text
JR_BRAIN event=autonomous_starting engine=esp-sr-wakenet
JR_BRAIN event=autonomous_on engine=esp-sr-wakenet model=wn9_hiesp wakeword=Hi,ESP sample_rate=16000 ...
JR_BRAIN event=listening wakeword=Hi,ESP model=wn9_hiesp
JR_BRAIN event=wakeword_detected source=voice keyword=Hi,ESP model=wn9_hiesp trigger=1
JR_BRAIN event=reaction_happy expression=feliz
JR_BRAIN event=listening wakeword=Hi,ESP model=wn9_hiesp
```

## Diagnostico

Se o modo autonomo iniciar, mas falar `Hi ESP` nao gerar evento:

- clicar **Simular evento (diagnostico)**;
- se a face ficar feliz, Local Brain/OLED estao corretos e o foco passa a ser microfone, nivel do sinal ou WakeNet;
- se a simulacao tambem falhar, o problema esta depois do reconhecimento.

Enquanto o modo autonomo estiver ativo, o barramento I2S fica reservado ao reconhecimento. Desative o modo autonomo antes de executar testes manuais de audio ou microfone.

## Proximo passo

Depois de validar `Hi ESP` fisicamente, solicitar/gerar um modelo WakeNet para a marca **JrBot** e substituir o modelo oficial de teste. Nao devemos renomear no log um modelo `Hi ESP` como se ele reconhecesse `JrBot`; o log sempre deve registrar o wake word real carregado.
