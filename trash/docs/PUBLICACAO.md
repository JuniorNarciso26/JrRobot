# JrBot — Public Presentation

## Identity

**JrBot** is an open-source ESP32-S3-based physical AI robot project.

Latest officially promoted stable release: **`JrBot_V1.7.04`**. The next defined product version is **JrBot V1.8S**, integrating JrSkill Network.

Suggested short description:

> Open-source ESP32-S3 physical AI robot with camera, audio, WebRTC, expressive OLED face and evolving Skill architecture.

## Project workstreams

- **V1 / `main`:** stable local-control product.
- **V1.8S / JrSkill Network:** integration of portable, declarative Skills, Solana-based licensing, and controlled physical execution; initially validated on Devnet.
- **V2:** local voice control.
- **V3:** future programmatic-control evolution.
- **JrBrain:** future memory, context, personality, and intelligence.

JrSkill implementation details and the original Colosseum evidence are retained on `V1s-00`.

## README and GitHub About

The root `README.md` is the repository's main public presentation. The GitHub About/Description field should remain short and should not claim that experimental features have already been promoted to `main`.

## JrSkill communication

When discussing JrSkill's implemented capabilities, distinguish Devnet validation from production launch. The architecture supports:

- Portable declarative Skills.
- Solana-backed publishing, versioning, and licensing.
- A safe local executor.
- Access to approved robot capabilities only.
- Integration with JrBot V1.8S, subject to final release promotion.

Do not describe a capability as available in the official stable `main` release while it is present only on `V1s-00`.

## Security and privacy

Before publishing logs, files, or demonstrations:

- Remove passwords, tokens, and credentials.
- Do not publish private audio or images without authorization.
- Do not expose the local portal directly to the Internet without a suitable authentication architecture.
- Preserve the licenses of dependencies and other third-party components.

## License

Original repository content is released under the [MIT License](../../LICENSE). The software license does not automatically convey rights to the **JrBot** trademark.
