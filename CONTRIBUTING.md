# Contributing to JrBot

Thank you for your interest in JrBot.

The project combines embedded firmware, electronic hardware, audio, computer vision, WebRTC, web interfaces, AI, and the **JrSkill Network** ecosystem for distributable robot Skills.

## Before you start

Read these documents:

1. [Documentation Index](docs/README.md)
2. [Current Project Status](docs/01-history-roadmap/PROJECT_STATUS.md)
3. [Roadmap](docs/01-history-roadmap/ROADMAP.md)
4. [Architecture](docs/04-firmware-development/ARCHITECTURE.md)
5. [Development Guide](docs/04-firmware-development/DEVELOPMENT.md)
6. [Testing and Validation](docs/04-firmware-development/TESTING.md)

## Project branches

- `main`: latest officially promoted stable release.
- `develop`: integration work.
- `V1s-00`: JrSkill Network/Solana integration and Colosseum history; preparing JrBot V1.8S.
- `v2`: local voice-control development.
- `v1`: historical V1 reference.

Do **not** develop directly on `main`.

### Contributing to the stable baseline

Start from `develop` and use a dedicated branch:

```text
feature/<name>
fix/<name>
hotfix/<name>
test/<experiment>
```

### Contributing to JrSkill Network

The integration line is `V1s-00`. It contains portable declarative Skills, Solana-backed distribution and licensing, and safe execution through local capabilities. Do not treat Devnet tests as evidence of production Mainnet availability.

### Contributing to local voice

Use `v2` for work specific to offline speech recognition or local voice commands.

## Contribution workflow

```text
Issue
→ research / architecture
→ branch
→ implementation
→ build
→ testing
→ physical validation
→ documentation
→ Pull Request
→ decision
```

Laboratory experiments should not be promoted directly to `develop`.

## Pull Requests

Describe:

- Purpose and related Issue.
- Source/base branch.
- Affected files and modules.
- Impact on hardware or shared resources.
- Build status.
- Tests performed.
- Physical validation completed or pending.
- Known limitations.
- Documentation updates.

## Hardware

Pin changes must update `hardware/pinmap.json` and the relevant documents. Before submitting:

```text
python tools/generate_pinmap.py --check
python tests/hardware/test_pin_policy.py
```

Document shared GPIO signals, power requirements, and peripheral conflicts explicitly.

## Evidence levels

Use the levels defined in [Testing and Validation](docs/04-firmware-development/TESTING.md):

- Planned.
- Implemented.
- Build verified.
- Bench/simulated.
- Physically validated.

Do not confuse successful software execution with physical confirmation.

## Security

- AI and external Skills cannot control GPIO directly.
- External actions must be allowlisted.
- Arguments must be validated.
- Never accept arbitrary executable code.
- Do not expose the local portal directly to the Internet.
- Do not publish credentials or personal information.

## License

JrBot is distributed under the [MIT License](LICENSE), subject to third-party license obligations. Accepted contributions use the same licensing terms.
