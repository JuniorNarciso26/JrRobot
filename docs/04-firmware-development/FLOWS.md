# JrBot Flow Engine — Future Design

**Status: PLANNED.** This document specifies a possible local automation system. It does **not** claim that the Flow Engine is available in the stable Runtime API.

## Concept

```text
TRIGGER → CONDITIONS → APPROVED ACTIONS
```

Illustrative behavior:

```text
voice("JR BOT") when confidence >= 0.58
→ face("happy") → say("hi") → wait(300) → listen()
```

## Proposed structured format

```json
{
  "schema": 1,
  "id": "wake_jrbot",
  "enabled": true,
  "trigger": {"fn": "voice", "args": {"phrase": "JR BOT", "confidence": 0.58}},
  "actions": [
    {"fn": "face", "args": {"expression": "happy"}},
    {"fn": "say", "args": {"content": "hi"}},
    {"fn": "wait", "args": {"ms": 300}},
    {"fn": "listen", "args": {}}
  ]
}
```

This is a **design example**, not a currently executable contract.

## Action and trigger plans

| Type | Candidates |
| --- | --- |
| **Actions** | `face`, `say`, `play`, `wait`, `listen`, `stopListen`, `camera.capture` |
| **Trigger** | `voice` |
| **Possible future triggers** | Timer, sensor event, validated API/VPS event |

Each action would require a registry entry with argument types, allowed values, resources and version. Movement actions require separate hardware, limits and safety validation.

## Constraints before implementation

- **Allowlist:** never execute arbitrary C, Python, shell commands or raw GPIO from Flow text.
- **Nonblocking:** `wait` schedules a continuation; it must not block critical tasks.
- **Concurrency:** define priorities, cancellation, timeout, failure handling and resource conflicts. One active local Flow at a time is the initial design preference.
- **Events:** plan `flow.started`, `flow.finished`, `flow.error`, `action.started`, `action.finished`, and `action.error`.
- **Versioning:** persist a `schema` version, reject unsupported data clearly, and preserve saved configuration for recovery.

**Implementation and physical validation remain pending.** For the functions currently available, consult [Runtime API](API_RUNTIME.md).
