# JrSkill Network — Hackathon Development Log

This log records the day-by-day development of **JrSkill Network** during the **Crypto World's Fair / Colosseum Hackathon 2026**.

The purpose is to keep a clear public timeline of what was planned, implemented, tested and validated during the hackathon.

JrBot itself predates the hackathon. This log focuses only on the JrSkill Network work developed during the competition.

---

## 2026-09-27 — Project kickoff / registration

The JrSkill Network project was registered for the Colosseum hackathon.

The initial concept was defined around a simple question:

> Can a physical robot receive portable Skills whose distribution and license state are represented on Solana, while the robot executes only safe, approved capabilities?

The first scope was intentionally narrow:

- use JrBot as the physical reference platform;
- keep the ESP32 isolated from arbitrary downloaded code;
- use declarative Skills instead of native firmware extensions;
- use Solana later for publication, retrieval and licensing;
- keep the first physical proof small and observable.

Technical tracking was centralized in **Issue #33**.

No Solana integration was implemented on this kickoff day.

---

## 2026-09-28 — Day 1: architecture and development plan

Day 1 focused on preparing the technical path before touching blockchain integration.

### Decisions

A dedicated experimental line was created:

```text
test/jrbot-v1s-00
firmware: JrBot_V1S_00
```

The V1S line was derived from the validated JrBot V1.7 baseline so the hackathon experiment could evolve without mixing with the stable product line.

The main architecture was defined as:

```text
Skill
  ↓
Skill Executor
  ↓
approved capability
  ↓
Runtime API
  ↓
JrBot hardware
```

A key decision was made early: the Skill must never contain arbitrary C/C++ code, direct GPIO access or executable binaries.

Instead, a Skill describes behavior and the local executor decides whether each requested capability is allowed.

### Development sequence

The planned proof path was organized as:

```text
local external Skill
        ↓
local executor
        ↓
physical JrBot
        ↓
Solana Skill storage
        ↓
Solana → executor → JrBot
        ↓
Wallet / License PDA
```

This order was chosen to avoid debugging blockchain, execution logic and hardware at the same time.

Relevant experimental branch start:

- `7cdb689c7525fe41ec405824782fc31b1d7a57c3` — initial `JrBot_V1S_00` variant.

Day 1 ended with the architecture and test direction defined. No blockchain proof was attempted yet.

---

## 2026-09-29 — Day 2: local Skill execution and physical validation

Day 2 focused on proving the execution model before moving to Solana.

### Step 1 — Runtime API baseline

An external JSON test sequence was added to the PC development panel.

The sequence used only capabilities that already existed in Runtime API 1.1:

- `capabilities`;
- `get`;
- `face`.

The test path was:

```text
external JSON
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

The physical robot executed:

```text
surprised → thinking → happy → neutral
```

The OLED transitions were visually confirmed and the test ended with:

```text
JR_SKILL_API_TEST result=ok
```

Relevant commits:

- `8625fb3f024a87719b1589c05d5847a9810009da` — Runtime API sequence through Serial;
- `904f922f5826703d780fc03a57ed760d49603414` — panel identification and test error handling.

### Step 2 — minimal JrSkill JSON v1

After the low-level path worked, the verbose test JSON was intentionally reduced to a minimal Skill format:

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

The current schema contains only:

- `v` — JrSkill schema version;
- `run` — ordered function calls.

Transport details, request IDs, firmware version, logs and expected test results are deliberately kept outside the executable Skill payload.

### Step 3 — reusable Recipes

A local Recipe library was introduced.

The first Recipe was:

```json
{
  "v": 1,
  "run": [
    ["face", "surprised"],
    ["wait", 500],
    ["face", "thinking"],
    ["wait", 500],
    ["face", "happy"],
    ["wait", 500]
  ]
}
```

This proved that a Skill can combine:

```text
direct function
+ local wait
+ reusable recipe
+ direct function
```

The executor resolves `recipe` locally and translates approved physical calls such as `face` into Runtime API requests.

Relevant implementation commit:

- `8b68a02863651105bd9814c979b86379cc4f65d4` — minimal JrSkill JSON v1 with local Recipe execution.

### Physical validation

The complete Skill was physically tested on the JrBot.

Observed behavior:

```text
happy
  ↓
wait 500 ms
  ↓
recipe face_sequence
    ├── surprised
    ├── thinking
    └── happy
  ↓
neutral
```

The face changes were visually confirmed on the physical OLED.

The executor completed with:

```text
JR_SKILL_V1 result=ok
```

### Day 2 result

The following pieces are now physically validated together:

- minimal JrSkill JSON v1;
- direct function calls;
- local `wait`;
- local `recipe` resolution;
- reusable Recipe blocks;
- Runtime API capability execution;
- observable behavior on physical JrBot hardware.

### Decision at the end of Day 2

The JrSkill JSON v1 is now **frozen for the next proof**.

No additional functions or schema complexity will be added before the first blockchain test.

The next development stage is intentionally postponed to **2026-09-30**.

Planned next proof:

```text
validated JrSkill JSON
        ↓
Solana Devnet
        ↓
Skill PDA
        ↓
retrieve the same JSON
        ↓
validate integrity
        ↓
existing JrSkill Executor
        ↓
physical JrBot
```

No Solana implementation was started on Day 2.

---

## Current checkpoint

At the end of 2026-09-29:

```text
LOCAL SKILL FORMAT        validated
LOCAL EXECUTOR            validated
LOCAL RECIPE              validated
RUNTIME API PATH          validated
PHYSICAL OLED EXECUTION   validated
SOLANA SKILL PDA          next stage
LICENSE PDA               later stage
```

The objective for the next development day is not to expand JrSkill. It is to prove that the already validated Skill can cross the blockchain boundary unchanged.

---

## References

- [JrSkill Execution API](JRSKILL_API.md)
- [Issue #33 — JrSkill Network / Solana Hackathon 2026](https://github.com/JuniorNarciso26/JrRobot/issues/33)
- [Experimental branch: test/jrbot-v1s-00](https://github.com/JuniorNarciso26/JrRobot/tree/test/jrbot-v1s-00)
- [Colosseum project page](https://colosseum.com/arena/projects/jrskill-network)
