# JrBot Firmware Development Guide

Use this guide to prepare the environment, build and flash firmware, and submit changes. The [Architecture](ARCHITECTURE.md), [Runtime API](API_RUNTIME.md) and [Testing Guide](TESTING.md) cover their own subjects.

## Environment

| Component | Requirement |
| --- | --- |
| Firmware toolchain | **ESP-IDF 5.5.x** for the documented development line |
| Target | ESP32-S3; **JRBOT-HW-04** for physical validation |
| Panel | Python 3 and dependencies from `tools/jrbot_frontend/requirements.txt` |
| Repository | Git and access to the selected branch |

**Keep ESP-IDF and panel Python environments separate.** `PAINEL.bat` uses `%LOCALAPPDATA%\JrBot\panel-venv` through the Windows `py -3` launcher. Never install panel requirements into the ESP-IDF private `python_env`.

An October 6, 2026 build failed because the panel had installed `cryptography 50.0.1` in the ESP-IDF 5.5.5 environment, which required `cryptography<45,>=2.1.4`. The isolated panel environment prevents recurrence. If ESP-IDF was already modified, repair its environment before building.

## Branches

| Branch | Purpose |
| --- | --- |
| `main` | Official stable release; most recently `JrBot_V1.7.04` |
| `V1s-00` | Completed V1.8S development; documentation/release preparation |
| `develop` | Integration baseline |
| `v2` | Experimental local voice control |
| `v1` | Historical V1 reference |

Do not push directly to `main`. Promotion requires explicit approval and release checks. For current release status, always use [Project Status](../01-history-roadmap/PROJECT_STATUS.md).

## Build, flash and open the panel

From the repository root on Windows:

```bat
INSTALAR.bat build
INSTALAR.bat flash
PAINEL.bat
```

The installer can synchronize a chosen branch. Check **which branch and firmware identifier** are active before proceeding; select the USB/Serial port reported by your computer rather than assuming a fixed number.

For manual branch preparation:

```bash
git switch V1s-00
git pull --ff-only origin V1s-00
```

Or use `develop` or `v2` only for work specific to those branches. A successful build proves compilation, **not** real-world behavior.

## Change workflow

```text
Issue → design → implementation → build → tests → documentation → review → approved promotion
```

Use a focused working branch when needed; do not create unrelated branches for the same feature. Keep experimental work away from `main` and avoid mixing V2 voice experiments with V1.8S release preparation.

Before changing hardware pins:

```bash
python tools/generate_pinmap.py --check
python tests/hardware/test_pin_policy.py
```

The authoritative pin map is `hardware/pinmap.json`; document changes in [Wiring](../02-assembly/WIRING.md).

## Documentation and Pull Requests

Each change should record: purpose, affected modules, build and test results, **physical validation or explicit pending status**, known limitations, and links to its Issue/PR. Do not call a feature physically tested based on logs or simulations alone.

For a public API, document functions, arguments, returns, errors and security constraints. For hardware changes, update the JSON pin map, generated definitions, wiring guide and tests. Maintain active documentation **in English** and preserve literal code, identifiers and JSON examples.

Update [Project Status](../01-history-roadmap/PROJECT_STATUS.md) only for the **current situation**, the [Changelog](../01-history-roadmap/CHANGELOG.md) for **completed changes**, and the [Roadmap](../01-history-roadmap/ROADMAP.md) for **future work**. Move outdated reference materials into `trash/` for review rather than erasing their history.
