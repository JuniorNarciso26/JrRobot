# JrBot Development Guide

## Prerequisites

- ESP-IDF 5.5.x for the current line.
- Python available in the development environment.
- Panel dependencies listed in `tools/jrbot_frontend/requirements.txt`.
- `JRBOT-HW-04`-compatible hardware for physical validation.

## Project branches

| Branch | Purpose |
| --- | --- |
| `main` | Official stable release — `JrBot_V1.7.04` |
| `develop` | Integration before the next promotion |
| `V1s-00` | JrSkill Network / Solana integration and Colosseum development history |
| `v2` | Local voice control |
| `v1` | Historical V1 reference |

`V1s-00` is a dedicated project development line, not an arbitrary temporary feature branch. Its integration into `main` must be explicitly approved and validated. The next planned product version is **JrBot V1.8S**.

## Standard development workflow

For work derived from the official baseline:

```text
IDEA
→ RESEARCH
→ ISSUE
→ ARCHITECTURE
→ ISOLATED BRANCH
→ PROOF
→ BUILD
→ PHYSICAL TEST
→ DOCUMENTATION
→ develop
→ main
```

Avoid mixing unrelated experiments in one branch. `test/*` branches are laboratories and should not be promoted directly to `develop`. Once a proof works, clean the solution and integrate it through the appropriate reviewed branch.

## Work on the integration baseline

```bash
git switch develop
git pull --ff-only origin develop
```

New feature:

```bash
git switch -c feature/<name>
```

Hotfix:

```bash
git switch -c hotfix/<name>
```

## Work on JrSkill / Solana

```bash
git switch V1s-00
git pull --ff-only origin V1s-00
```

Do not copy experimental V1S changes into `main` or `develop` automatically. Preserve the original development evidence until integration is approved.

## Work on local voice (V2)

```bash
git switch v2
git pull --ff-only origin v2
```

The local voice workstream remains separate from the JrSkill workstream.

## Hardware checks

Before changing the pin map:

```text
python tools/generate_pinmap.py --check
python tests/hardware/test_pin_policy.py
```

The machine-readable source of truth remains `hardware/pinmap.json`.

## Build

```bat
INSTALAR.bat build
```

A successful build proves **compilation**, not physical operation.

## Flash

```bat
INSTALAR.bat flash
```

After flashing, confirm the exact firmware identifier in the device log/status.

## Physical testing

Record the installed build/version, scenario, observed behavior, relevant logs, result, and known limitations. Never treat an `ESP_OK` response, HTTP success, JPEG frame, or log message alone as proof of an observed physical outcome.

## Pull Requests

Every PR should document its purpose, related Issue when applicable, source branch and base, affected modules, hardware/resource impacts, build status, tests, physical validation, known limitations, and updated documentation.

## Documentation updates

When integrating a meaningful change, review the applicable files:

- Root `README.md`.
- `docs/01-history-roadmap/ROADMAP.md`.
- `docs/01-history-roadmap/PROJECT_STATUS.md`.
- `docs/04-firmware-development/ARCHITECTURE.md`.
- `docs/01-history-roadmap/CHANGELOG.md`.
- Any affected technical documents.

Move obsolete material to `trash/` for review rather than immediately deleting its history.

## Main repository structure

```text
firmware/
  core/
  main/
  module_audio/
  module_brain/
  module_camera/
  module_face/
  module_portal/
  module_runtime/

tools/jrbot_frontend/
hardware/
docs/
tests/
trash/              archived material awaiting review
```
