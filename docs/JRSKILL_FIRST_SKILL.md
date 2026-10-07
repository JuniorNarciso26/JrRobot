# Build your first JrSkill

> **Pilot v0 — JrSkill Developer Challenge**
>
> **Build a JrSkill. We'll run it on a real robot.**

This guide is the public onboarding path for the first JrSkill contributors.

You do **not** need to own a JrBot, an ESP32 board, a Solana wallet, or a robotics lab to create a Skill in this pilot.

Your job is to describe a small robot behavior using the JrSkill JSON v1 format. The JrBot maintainer validates the submission and, if it passes the safety/format checks, runs it on the physical JrBot and records the result.

## 1. What is a JrSkill?

A JrSkill is a small declarative JSON document that describes **what the robot should do**.

It does not contain C/C++, shell commands, raw GPIO access, memory addresses, binaries, or arbitrary executable code.

The execution boundary is:

```text
JrSkill JSON
    ↓
schema / allowlist validation
    ↓
JrSkill Executor
    ↓
Runtime API
    ↓
approved capability
    ↓
physical JrBot
```

For the first public pilot, keep the Skill intentionally simple.

## 2. Pilot v0 scope

The first onboarding test accepts only:

- `face` — change the JrBot OLED expression;
- `wait` — wait before the next call.

Although the current executor also supports local Recipes, **do not use `recipe` in pilot submissions**. Recipes depend on the maintainer's local library and would make the first external-author test less portable.

Current limits enforced by the executor:

- schema version must be `v: 1`;
- `run` must contain at least 1 call;
- maximum 64 top-level calls;
- each `wait` must be an integer from **0 to 5000 ms**;
- only allowlisted functions are accepted;
- unknown functions stop execution.

## 3. Available `face` values

Use the English expression names below:

```text
neutral
happy
sad
excited
angry
surprised
thinking
skeptical
sleepy
confused
winking
love
playful
worried
cool
battery
```

Example single action:

```json
["face", "happy"]
```

## 4. The JSON format

Every pilot Skill must contain the JrSkill v1 execution structure:

```json
{
  "v": 1,
  "run": [
    ["face", "happy"],
    ["wait", 500],
    ["face", "neutral"]
  ]
}
```

This is an example only. For the pilot, create your own sequence instead of submitting this example unchanged.

### `face`

Format:

```json
["face", "expression"]
```

Example:

```json
["face", "surprised"]
```

### `wait`

Format:

```json
["wait", milliseconds]
```

Example:

```json
["wait", 750]
```

The value must be an integer between 0 and 5000.

## 5. Create your Skill

Use any plain-text editor.

Create one JSON file using this naming convention:

```text
<github-user>-<skill-name>.json
```

Example:

```text
alice-hello-robot.json
```

Recommended rules for the Skill name:

- lowercase;
- short and descriptive;
- use `-` between words;
- do not reuse the name of an existing submission.

Your Skill should make the physical result easy to recognize in a short video.

A good first Skill normally has:

- 3–10 face changes;
- short waits between expressions;
- a clear beginning and ending state;
- `neutral` as the final expression when appropriate.

## 6. Self-check before submitting

Before opening a submission, confirm:

- [ ] the file is valid JSON;
- [ ] `v` is exactly `1`;
- [ ] `run` is a non-empty array;
- [ ] every call is either `face` or `wait`;
- [ ] every face value is listed in this guide;
- [ ] every wait is an integer from 0 to 5000;
- [ ] there are no more than 64 calls;
- [ ] there is no C/C++, shell command, GPIO access, script, URL, binary, or arbitrary executable content;
- [ ] the behavior is visibly different from the example above.

## 7. Submit the Skill

For pilot v0, submit through a new GitHub Issue in:

**JuniorNarciso26/JrRobot**

Use this title:

```text
[JrSkill submission] <skill-name>
```

Use this body:

```markdown
## Author

GitHub: @YOUR_GITHUB_USER

## Skill name

YOUR_SKILL_NAME

## Intended behavior

Describe in one or two sentences what the JrBot should visibly do.

## JrSkill JSON

```json
PASTE_YOUR_JSON_HERE
```

## Attribution

How should your name / GitHub username appear in the public demo?

## Pilot feedback

Time spent creating the Skill:
Unclear step, if any:
Suggestion, if any:
```

Do not include private keys, seed phrases, passwords, API keys, personal addresses, or other private information.

## 8. What happens after submission?

The pilot validation path is:

```text
submission
    ↓
JSON/schema review
    ↓
allowlist and capability review
    ↓
local JrSkill Executor validation
    ↓
physical JrBot execution
    ↓
log / video / evidence
    ↓
result recorded in the submission
```

A submission may be rejected or returned for correction if it is malformed, uses an unsupported function, or creates an unsafe/unbounded behavior.

The maintainer should report evidence levels separately:

- **static validation** — format/code inspection only;
- **executor validation** — Skill accepted/exercised by the executor;
- **physical test** — observable behavior confirmed on the real JrBot.

Do not describe a Skill as physically validated until the hardware test actually happens.

## 9. Solana status in pilot v0

Creating a JrSkill does **not automatically publish it on Solana**.

The first Developer Challenge experiment is intentionally testing the lowest-friction question first:

> Can an external developer understand the format, create a capability, submit it, and see it executed on a real robot?

The JrSkill Network already has a validated Solana Devnet Skill PDA and License PDA flow, but onboarding new publishers is a separate product step.

If a pilot Skill is selected for an on-chain experiment, publication/licensing will be recorded separately with its actual transaction/account evidence.

## 10. What success looks like

A successful first contribution produces:

```text
developer
   ↓
original JrSkill JSON
   ↓
accepted validation
   ↓
physical JrBot behavior
   ↓
public evidence + attribution
```

The first maintainer-run submission is an **internal onboarding test** and does not count as external traction.

After that test passes, the same guide will be used with external contributors.

## References

- [JrSkill Execution API](JRSKILL_API.md)
- [Runtime API v1](API_RUNTIME.md)
- [JrSkill traction / Developer Challenge — Issue #52](https://github.com/JuniorNarciso26/JrRobot/issues/52)
- [JrSkill technical tracking — Issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33)
- [Current experimental branch — V1s-00](https://github.com/JuniorNarciso26/JrRobot/tree/V1s-00)
