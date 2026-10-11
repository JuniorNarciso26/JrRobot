# Python Environments for the JrBot Panel and ESP-IDF

The JrBot panel and ESP-IDF require different Python dependencies and therefore must use **separate environments**.

## Rule

- **ESP-IDF:** use the environment created by the official Espressif installer.
- **JrBot panel:** use `%LOCALAPPDATA%\JrBot\panel-venv`.
- **Never** install `tools/jrbot_frontend/requirements.txt` into `C:\Espressif\python_env\...`.

## Incident — October 6, 2026

During the first attempt to build `JrBot_V1S_APP_01`, ESP-IDF 5.5.5 refused to start because its Python environment contained `cryptography 50.0.1`, while the official constraint required `cryptography<45,>=2.1.4`.

**Cause:** `PAINEL.bat` selected the first `python` executable in PATH. When opened after the ESP-IDF `export.bat`, that executable pointed to ESP-IDF's private Python installation. As a result, the panel script installed its dependencies into the ESP-IDF environment.

**Fix:** `PAINEL.bat` now creates and uses a dedicated virtual environment outside the repository through the Windows `py -3` launcher.

## Status

The change prevents future cross-contamination. An ESP-IDF environment that was already modified must be repaired once before the next build.
