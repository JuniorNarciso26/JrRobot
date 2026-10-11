# JrBot Runtime API v1

## Current contract

Protocol on the stable `main` baseline:

```text
major protocol: 1
implementation: 1.1
```

The Runtime API separates clients and interfaces from the firmware's physical implementations. It accepts **registered functions only** and does not execute arbitrary text, native code, or direct GPIO commands.

## Implemented functions

The stable baseline registers:

- `capabilities` — query.
- `get` — query.
- `face` — action.

Dynamic discovery also reports available transports.

## Request and response envelope

Request:

```json
{
  "v": 1,
  "id": "req01",
  "fn": "capabilities",
  "args": {}
}
```

Response:

```text
JR_API {"v":1,"ok":true,"id":"req01","result":{...}}
```

Errors use the same envelope with `ok=false`. The `id` field is optional and allows clients to correlate requests and responses.

## Transports

### Serial

```text
api {"v":1,"fn":"capabilities","args":{}}
```

### Local HTTP

```text
POST /cmd
X-JrBot-Command: 1
Content-Type: text/plain
```

Body:

```text
api {"v":1,"fn":"capabilities","args":{}}
```

The current HTTP command interface is intended for a trusted local network. The `X-JrBot-Command: 1` header is **not authentication** for public Internet exposure.

## `capabilities`

The response describes the API version, protocol, firmware, hardware, profile, functions, `get` paths, actions, transports, and flags for features that are not yet available.

Stable baseline:

```text
functions = capabilities, get, face
actions   = face
persistent_config = false
flow_engine       = false
playground        = false
```

## `get`

Registered paths:

```text
api.version
system.version
system.hardware
system.profile
brain.status
voice.status
audio.volume
face.current
wifi.status
wifi.diagnostics
```

Example:

```json
{
  "v": 1,
  "fn": "get",
  "args": { "path": "system.version" }
}
```

## `face`

The `face` action accepts expressions supported by the OLED renderer:

```json
{
  "v": 1,
  "fn": "face",
  "args": { "expression": "happy" }
}
```

This invokes only the registered firmware face module. Unknown expressions produce a structured error and do not authorize direct access to the display or GPIO.

## Structured errors

| Code | Meaning |
| --- | --- |
| `invalid_json` | Invalid JSON body |
| `invalid_request` | Invalid request envelope |
| `invalid_args` | Incompatible arguments |
| `unsupported_version` | Unsupported major version |
| `not_found` | Function/path unavailable |
| `no_memory` | Allocation or serialization failure |
| `response_too_large` | Response exceeds the buffer |

## Security

The Runtime API enforces a function allowlist. Its contract does **not** permit arbitrary C/C++ execution, a shell, raw GPIO access, direct memory access, or unknown function names.

These restrictions are also the foundation for future voice, JrBrain, and JrSkill Network integration.

## Compatibility and evolution

Compatible capabilities may continue to use `v=1`, and must appear in `capabilities()`. Breaking changes require a new major protocol version.

Older candidate specifications and test plans were moved to `trash/`; they do not define the current contract.
