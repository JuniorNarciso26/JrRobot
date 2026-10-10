# 2026-09-28 — Day 1: architecture and development plan

[Back to the development log index](../HACKATHON_DEVLOG.md)


Day 1 focused on preparing the technical path before touching blockchain integration.

## Decisions

A dedicated experimental line was created:

```text
V1s-00
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

## Development sequence

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

[Previous: Kickoff](Kickoff.md) · [Next: Day 2](day2.md)

## References

- [JrSkill Execution API](../JRSKILL_API.md)
- [Issue #33 — JrSkill Network / Solana Hackathon 2026](https://github.com/JuniorNarciso26/JrRobot/issues/33)
- [Experimental branch: V1s-00](https://github.com/JuniorNarciso26/JrRobot/tree/V1s-00)
- [Colosseum project page](https://colosseum.com/arena/projects/jrskill-network)
