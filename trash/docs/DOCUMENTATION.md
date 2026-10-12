# Documentation Conventions

## Purpose

The documentation should allow contributors to understand the JrBot project's state without access to the maintainer's chat history.

## Source of truth

Project documentation belongs in `docs/`. The local panel renders these documents; it does not maintain separate copies of their contents.

## Every feature must declare its status

Use these evidence levels:

```text
PLANNED
IMPLEMENTED
BUILD VERIFIED
BENCH / SIMULATED
PHYSICALLY VALIDATED
```

Different parts of the same document may have different statuses.

## Document a capability

Include:

1. Purpose.
2. Public name.
3. Arguments.
4. Allowed values.
5. Return value.
6. Errors.
7. Physical resources used.
8. Concurrency.
9. Persistence.
10. Events/logs.
11. Security.
12. Test procedure.
13. Current status.

## Document an API

Separate its logical contract, transport, authentication/security, versioning, examples, and implementation status.

Do not confuse a human-friendly Flow syntax with arbitrary code execution.

## Document hardware

Update these references together:

- `hardware/pinmap.json`.
- `docs/02-assembly/WIRING.md` (wiring and GPIO reference).
- GPIO tests and policies.

## Future Architecture Decision Records (ADRs)

Important architectural decisions may eventually move to `docs/decisions/` as short records covering **context, alternatives, decision, and consequences**.

Examples: Flow storage format, official Runtime API transport, portal authentication, final wake-word mechanism, and TTS strategy.

## Language

**English is the canonical language for active documentation in `docs/` and for the repository's public README.** Preserve machine-readable identifiers, program names, logs, and literal API payloads. Historical source records should be kept intact when serving as evidence; user-facing navigation and release information must be in English.
