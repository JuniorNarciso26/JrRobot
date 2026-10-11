# JrBot Flow Engine

**Status: PLANNED / initial specification.**

Flows describe behavior without recompiling firmware.

## Model

```text
TRIGGER
   ↓
CONDITIONS
   ↓
ACTIONS
```

Example:

```text
voice("JR BOT")
confidence >= 0.58
-> face("happy")
-> say("hi")
-> wait(300)
-> listen()
```

## Structured representation

```json
{
  "schema": 1,
  "id": "wake_jrbot",
  "enabled": true,
  "trigger": {
    "fn": "voice",
    "args": {
      "phrase": "JR BOT",
      "confidence": 0.58
    }
  },
  "actions": [
    {"fn": "face", "args": {"expression": "happy"}},
    {"fn": "say", "args": {"content": "oi"}},
    {"fn": "wait", "args": {"ms": 300}},
    {"fn": "listen", "args": {}}
  ]
}
```

The JSON retains the original sample payload as historical data.

## Why structured JSON rather than arbitrary code?

Firmware must execute **registered, validated functions only**.

Never:

```text
receive text -> interpret C/Python -> write GPIO
```

Always:

```text
Flow -> action registry -> validation -> safe capability
```

## Action Registry

Each function requires metadata.

Conceptual example:

```json
{
  "name": "face",
  "version": 1,
  "args": {"expression": "enum"},
  "resource": "oled"
}
```

This enables `capabilities()` and pre-save Flow validation.

## Proposed initial actions

- `face`
- `say`
- `play`
- `wait`
- `listen`
- `stopListen`
- `camera.capture`

Movement actions may only be added after their physical drivers and limits are implemented and validated.

## Proposed initial triggers

- `voice`
- Future: local timer.
- Future: sensor event.
- Future: safe event received through an API/VPS.

## Execution

The Flow Engine must not block critical tasks. `wait(5000)` means schedule the Flow's continuation, **not** suspend essential processing for five seconds.

## Concurrency

Before implementation, define whether one or multiple Flows can run simultaneously, priority, cancellation, timeout, conflicts over audio/camera/motors, and behavior on action failure.

For the first release, **one local Flow at a time** is the simplest predictable choice.

## Errors

A Flow should emit structured events:

```text
flow.started
flow.finished
flow.error
action.started
action.finished
action.error
```

When an action fails, explicitly define whether the Flow aborts, continues, or runs a fallback. The schema should express that policy.

## Versioning

Every persisted Flow must have a `schema` version. New firmware that cannot interpret an older schema must reject it clearly and preserve its saved configuration for recovery, rather than executing partially understood behavior.
