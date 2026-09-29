# JrSkill Execution API

**Status:** Hackathon architecture document for **JrSkill Network — Crypto World's Fair / Colosseum 2026**.

**Current experimental line:** `JrBot_V1S_00` on `V1s-00`.

This document is the current reference for how a JrSkill is represented, resolved and executed. Older Runtime API candidate/test documents are kept as technical history.

## 1. Goal

JrSkill Network separates **what a Skill wants to do** from **how the robot hardware executes it**.

A Skill is a small declarative JSON payload. It never contains arbitrary C/C++ code, GPIO access, memory addresses or executable binaries.

The execution path is:

```text
JrSkill JSON
    ↓
JrSkill Executor
    ↓
direct function / wait / recipe
    ↓
Runtime API and approved local services
    ↓
JrBot hardware
```

The same Skill format is intended to work whether the Skill comes from a local file, a future public/private library, or Solana.

## 2. Minimal JrSkill JSON v1

The next test uses the following minimal execution format:

```json
{
  "v": 1,
  "run": [
    ["face", "happy"],
    ["wait", 500],
    ["recipe", "sound_party"],
    ["face", "neutral"]
  ]
}
```

Only two top-level fields are required:

- `v` — JrSkill schema version.
- `run` — ordered list of function calls.

Each item in `run` is a function call:

```text
["function", argument1, argument2, ...]
```

Examples:

```json
["face", "happy"]
["wait", 500]
["recipe", "sound_party"]
```

The payload deliberately does **not** repeat test, transport or metadata fields such as:

- description;
- source;
- Serial/HTTP transport;
- firmware build;
- request correlation IDs;
- expected test responses;
- logs;
- Runtime API envelope for every call.

Those belong to the executor, test harness or on-chain metadata, not to the Skill execution payload.

## 3. Direct functions

A direct function maps to an approved robot capability.

Example:

```json
["face", "happy"]
```

The executor translates that call into the low-level Runtime API request:

```json
{
  "v": 1,
  "fn": "face",
  "args": {
    "expression": "happy"
  }
}
```

The Skill therefore stays compact while the executor owns protocol details such as request IDs, transport and response validation.

## 4. Executor functions

Some functions belong to the Skill Executor instead of the ESP32 hardware API.

The first example is:

```json
["wait", 500]
```

This means: wait 500 ms before running the next item.

No hardware command is required for the delay itself.

## 5. Recipes

A Recipe is a reusable block of JrSkill calls.

A Skill can mix new direct calls and existing Recipes:

```json
{
  "v": 1,
  "run": [
    ["face", "happy"],
    ["wait", 500],
    ["recipe", "sound_party"],
    ["face", "neutral"]
  ]
}
```

The first implementation will resolve Recipes from a local library in the PC/App/JrTK layer.

Example Recipe:

```json
{
  "v": 1,
  "run": [
    ["tone", 880, 200],
    ["wait", 100],
    ["tone", 1200, 200],
    ["wait", 100],
    ["tone", 1500, 300]
  ]
}
```

Execution becomes:

```text
Skill
  ↓
recipe("sound_party")
  ↓
Library Resolver
  ↓
Recipe JSON
  ↓
tone / wait / tone / ...
  ↓
Runtime API + Executor
```

Recipes may later be public, private, versioned and distributed through JrSkill Network. Recursive Recipe calls will require cycle detection and a maximum nesting depth before being enabled.

## 6. Runtime API

The current firmware exposes Runtime API major `v=1`, implementation version `1.1`.

The currently implemented functions used by the V1S baseline are:

- `capabilities`;
- `get`;
- `face`.

Current Serial transport:

```text
api {"v":1,"fn":"face","args":{"expression":"happy"}}
```

Successful responses use a structured envelope:

```text
JR_API {"v":1,"ok":true,"result":{...}}
```

The executor is responsible for converting compact JrSkill calls into this Runtime API form.

## 7. Capability discovery

A Skill should not be tied to a specific firmware build such as `JrBot_V1S_00`.

Instead, the executor queries:

```text
capabilities()
```

and verifies that the robot supports the functions required by the Skill or Recipe.

This allows the same Skill to remain usable across future compatible JrBot firmware versions and other compatible devices.

## 8. Security model

JrSkill execution is allowlisted.

A Skill may call only functions registered by the executor/Runtime API.

The model explicitly rejects:

- arbitrary native code;
- arbitrary shell commands;
- raw GPIO access;
- raw memory access;
- unknown function names;
- unbounded Recipe recursion.

The intended boundary is:

```text
external Skill data
      ↓
schema validation
      ↓
function / Recipe resolution
      ↓
capability validation
      ↓
Runtime API
      ↓
hardware
```

## 9. Solana separation

The executable Skill JSON is intentionally minimal.

Publication and ownership information belongs outside the execution payload.

Conceptually:

```text
Skill PDA
├── publisher
├── skill identifier
├── version
├── content hash
├── status
└── content / content reference

License PDA
└── wallet + skill authorization

Executable payload
└── {"v":1,"run":[...]}
```

This avoids repeating metadata inside every Skill and keeps the on-chain execution payload small.

## 10. Physically validated baseline

On **2026-09-29**, the `JrBot_V1S_00` experimental firmware was physically exercised through the Serial test panel.

Validated path:

```text
external JSON test sequence
    ↓
PC test panel
    ↓
Serial
    ↓
Runtime API 1.1
    ↓
face capability
    ↓
SSD1306 OLED
```

The observed OLED sequence was:

```text
surprised → thinking → happy → neutral
```

The test also queried `face.current` after the transitions and completed with:

```text
JR_SKILL_API_TEST result=ok
```

This proves the current low-level execution path. It does **not** yet prove the final JrSkill v1 executor, Recipe resolver, Solana integration or license flow.

## 11. Current implementation status

| Component | Status |
| --- | --- |
| Runtime API `capabilities` | Implemented |
| Runtime API `get` | Implemented |
| Runtime API `face` | Implemented and physically validated in V1S |
| Serial Runtime API transport | Implemented and physically validated |
| Minimal JrSkill JSON v1 | Architecture defined for next test |
| JrSkill Executor | Next implementation step |
| `wait` executor function | Planned for next test |
| Local Recipe resolver | Planned for next test |
| Runtime API audio/tone action | Planned |
| Public/private Recipe libraries | Future evolution |
| Solana Skill PDA | Planned for hackathon MVP |
| Solana License PDA | Planned for hackathon MVP |
| Wallet ownership validation | Planned for hackathon MVP |

## 12. Next proof

The next isolated experiment should validate the new compact JrSkill JSON without changing the Solana layer yet:

```json
{
  "v": 1,
  "run": [
    ["face", "happy"],
    ["wait", 500],
    ["recipe", "face_sequence"],
    ["face", "neutral"]
  ]
}
```

The Recipe should initially use only already validated `face` operations. After the resolver is physically proven, audio/tone can be added as the next Runtime API capability.

## References

- Technical tracking: [Issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33)
- Experimental branch: [V1s-00](https://github.com/JuniorNarciso26/JrRobot/tree/V1s-00)
- Colosseum project: https://colosseum.com/arena/projects/jrskill-network
- Low-level Runtime API history: [API_RUNTIME.md](API_RUNTIME.md)
