# JrBot Runtime API v1

The Runtime API is the firmware's **registered-function interface**. It does not execute arbitrary text or direct GPIO operations.

## Contract

| Property | Value |
| --- | --- |
| Protocol major | `1` |
| Implementation | `1.1` |
| Stable registered functions | `capabilities`, `get`, `face` |
| Response prefix | `JR_API ` |
| Availability | Stable V1.7.04 baseline; V1.8S integration extends the wider ecosystem |

## Calls and parameters

| Function | Purpose | Arguments |
| --- | --- | --- |
| `capabilities` | Discover registered functions, paths, transports and feature flags | `{}` |
| `get` | Read an allowed property | `{"path":"system.version"}` |
| `face` | Set an expression using the OLED module | `{"expression":"happy"}` |

Available `get` paths:

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

The exact supported OLED expressions are defined by the firmware's face renderer. Unknown values must return an error rather than bypass validation.

## Request and response

Example request:

```json
{"v":1,"id":"req01","fn":"face","args":{"expression":"happy"}}
```

Example response envelope:

```text
JR_API {"v":1,"ok":true,"id":"req01","result":{...}}
```

The optional `id` links a response to its request; failures use `ok=false` and a structured `error`.

## Transports

**Serial command:**

```text
api {"v":1,"fn":"capabilities","args":{}}
```

**Local HTTP command:**

```text
POST /cmd
X-JrBot-Command: 1
Content-Type: text/plain

api {"v":1,"fn":"capabilities","args":{}}
```

**Security:** `X-JrBot-Command: 1` is a command indicator, **not an authentication mechanism**. The portal's local HTTP API must not be exposed directly to the Internet as a secure remote-control service.

## Error codes

| Code | Meaning |
| --- | --- |
| `invalid_json` | Cannot parse JSON |
| `invalid_request` | Invalid request envelope |
| `invalid_args` | Arguments are incompatible |
| `unsupported_version` | Unsupported protocol major |
| `not_found` | Unknown function/path |
| `no_memory` | Memory or serialization failure |
| `response_too_large` | Buffer too small for the response |

## Capability negotiation

The stable `capabilities` response reports `persistent_config=false`, `flow_engine=false`, and `playground=false`. Plans for those features in [Flows](FLOWS.md) and [Playground](PLAYGROUND.md) must **not** be mistaken for implemented Runtime API functions.

Maintain the major version for compatible additions and declare new functions in `capabilities()`. Breaking protocol changes require a new major version. No client may invoke unregistered functions, arbitrary code, a shell, raw GPIO or memory.
