# JrBot Runtime API v1

**Status: PLANEJADO / especificação de arquitetura.**

Este documento define a direção do contrato público do JrBot. As funções abaixo não devem ser consideradas disponíveis no firmware atual até serem marcadas como implementadas em `PROJECT_STATUS.md`.

## Objetivo

Separar capacidades do firmware da interface que as utiliza.

```text
Painel ----\
CLI --------> JrBot Runtime API -> capabilities seguras
VPS -------/
```

## Formato lógico

A API pública será orientada a funções e eventos. Transporte (Serial, HTTP local, futuro WebSocket etc.) é uma decisão separada.

Uma função conceitual:

```text
face("happy")
```

pode ser transportada como JSON sem executar código textual arbitrário:

```json
{
  "fn": "face",
  "args": {
    "expression": "happy"
  }
}
```

## Funções de base propostas

### `get(path)`

Lê estado.

```text
get("system.version")
get("brain.status")
get("voice.status")
get("audio.volume")
get("face.current")
get("wifi.status")
get("camera.status")
```

### `set(path, value, persist=false)`

Altera uma configuração controlada.

```text
set("audio.volume", 35)
set("brain.autonomous", true)
```

### `say(content)`

Solicita uma resposta falada.

Primeira implementação pode aceitar apenas assets/respostas locais conhecidas, como `oi`. TTS arbitrário é uma capacidade separada e futura.

```text
say("oi")
```

### `face(expression, duration_ms?)`

```text
face("happy")
face("thinking", duration_ms=1500)
```

### `wait(ms)`

Pausa um Flow sem bloquear tarefas críticas do firmware.

### `listen(mode?, timeout_ms?)`

Coloca o motor de voz em um modo de escuta suportado.

### `stopListen()`

Interrompe uma sessão de escuta controlada.

### `play(asset)`

Reproduz um asset de áudio conhecido e validado.

### `camera(operation)`

Primeira operação prevista:

```text
camera("capture")
```

## Voice API proposta

```text
voice.add(phrase)
voice.remove(id)
voice.list()
voice.get(id)
```

Configuração de confiança deve ser associada ao trigger/regra, não necessariamente ao threshold global interno do modelo.

## Playground API proposta

```text
playground.start(phrase, interval_ms)
playground.stop()
playground.save(confidence)
```

Resultados são eventos, por exemplo:

```json
{
  "event": "playground.result",
  "session": "pg_001",
  "sequence": 14,
  "expected": "JR BOT",
  "recognized": "JR BOT",
  "confidence": 0.624,
  "timestamp_ms": 487221
}
```

## Flow API proposta

```text
flow.save(...)
flow.list()
flow.get(id)
flow.delete(id)
flow.enable(id)
flow.disable(id)
flow.run(id)
```

## Descoberta de capabilities

O painel não deve codificar para sempre uma lista fixa de funções.

```text
capabilities()
```

Resposta conceitual:

```json
{
  "api": "1.0",
  "actions": ["say", "face", "wait", "listen", "play", "camera.capture"],
  "triggers": ["voice"],
  "faces": ["neutral", "happy", "sad", "thinking", "cool"]
}
```

Quando uma nova capability for adicionada ao firmware, clientes podem descobri-la sem assumir que todas as versões possuem o mesmo conjunto.

## Eventos propostos

```text
voice.candidate
voice.accepted
voice.rejected
playground.started
playground.result
playground.stopped
flow.started
flow.finished
flow.error
action.started
action.finished
action.error
```

## Requisitos de compatibilidade

Toda futura API deve informar:

- versão do protocolo;
- capability disponível;
- argumentos aceitos;
- erro estruturado;
- se a operação é persistente;
- se a função exige recurso físico ocupado.

Mudanças incompatíveis exigem nova versão de contrato.
