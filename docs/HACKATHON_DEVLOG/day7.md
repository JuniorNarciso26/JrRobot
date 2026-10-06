# Day 7 — embedded JrBot App validates licensed Skill flow

Date: 2026-10-06. Branch: `V1s-00`. Firmware: **`JrBot_V1S_APP_02`**. Solana Devnet/test SOL.

## Goal

Move the already validated Wallet / license / Skill flow out of the development panel and into the **existing local App served by the JrBot firmware**, without moving purchase/marketplace responsibilities into the robot.

The App keeps the robot side focused on usage:

```text
Wallet
  -> authenticate
  -> verify Devnet
  -> discover licensed Skills
  -> select Skill
  -> execute on JrBot
```

Skill purchase remains intentionally outside the firmware and will be handled by a separate Web Store.

## Implementation

The existing JrBot App received a new **Minhas Skills** board with:

- Connect wallet;
- Authenticate wallet by signed message;
- Verify Solana Devnet and balance;
- Search licensed Skills;
- Select a Skill;
- Execute the selected Skill on the JrBot;
- Disconnect wallet.

The browser performs Wallet and Solana HTTPS operations. The ESP32 continues to serve the App and expose the controlled Runtime API; it does **not** become a direct Solana client.

## Installation incidents and fixes

### 1. ESP-IDF Python environment contamination

The first build attempt stopped before compilation because the ESP-IDF 5.5.5 environment contained `cryptography 50.0.1`, while the ESP-IDF constraints required `<45`.

Cause: the old `PAINEL.bat` could use the ESP-IDF Python from `PATH` and install the development-panel requirements into that private environment.

Fix:

- panel dependencies moved to an isolated virtual environment under `%LOCALAPPDATA%\JrBot\panel-venv`;
- the ESP-IDF environment was repaired with the official installer;
- ESP-IDF returned to `cryptography 44.0.3`.

### 2. ESP-IDF environment was not fully exported

The next attempt stopped because `IDF_PYTHON_ENV_PATH` was missing and CMake was not on `PATH`.

Cause: `INSTALAR.bat` detected an existing `IDF_PATH` and skipped the ESP-IDF `export.bat`.

Fix: the installer now always executes `export.bat` before build/flash and validates `IDF_PATH`, `IDF_PYTHON_ENV_PATH` and `cmake.exe`.

### 3. APP_01 installed but Minhas Skills did not appear

`JrBot_V1S_APP_01` was physically installed and identified correctly by the App, but the new board was absent.

Cause: `jrskill_app.js` was embedded with `EMBED_TXTFILES` and the HTTP handler sent the terminating NUL byte as part of the JavaScript response.

Fix: `JrBot_V1S_APP_02` serves the embedded text using `HTTPD_RESP_USE_STRLEN`, excluding the terminator.

### 4. HTTPS requirement confirmed

The Wallet flow was initially opened through plain HTTP and Chrome blocked the required secure browser/Wallet capabilities.

Using the JrBot App through:

```text
https://<JRBOT_IP>/
```

resolved the browser restriction. HTTPS is therefore part of the current App test requirement.

## Physical validation — APP_02

The user installed `JrBot_V1S_APP_02`, opened the App through HTTPS and confirmed the complete flow working.

Observed App state:

- **Wallet autenticada**;
- **Devnet confirmada**;
- licensed wallet address `6rinyiBQyS4RxLeuX62yJqxgwiJqWXiyAbxXSS5LX85R`;
- balance **3.9985738 test SOL**;
- `minimal_recipe_01 - licenciada`;
- payload **128 bytes**;
- hash prefix `416d6af34ead...`, matching the known JrSkill checkpoint hash;
- Skill selected;
- final message: **`Skill concluida no JrBot: result=ok`**.

The user confirmed the App and robot behavior as working correctly.

### Validation status

- **Compilation / installation:** completed on the user's environment;
- **Firmware identification:** `JrBot_V1S_APP_02` confirmed;
- **Embedded App board:** physically confirmed;
- **Wallet authentication:** confirmed;
- **Devnet verification:** confirmed;
- **License discovery:** confirmed;
- **Skill selection:** confirmed;
- **physical execution:** confirmed by the user with `result=ok`;
- **purchase inside the App:** intentionally absent.

## Product boundary — Store Web moves to another repository

Day 7 closes the robot-side App MVP for the current hackathon proof.

The purchase/catalog experience is a different product surface and will not be developed inside the firmware repository.

New tracking issue:

- [JrBot_Web #16 — JrSkill Store: Store Web para descobrir e comprar Skills](https://github.com/JuniorNarciso26/JrBot_Web/issues/16)

Separation:

```text
JrBot repository / V1s-00
  -> firmware
  -> embedded App
  -> Minhas Skills
  -> select / execute

JrBot_Web repository
  -> Store catalog
  -> Skill details
  -> Wallet
  -> pricing / terms
  -> purchase
  -> License PDA confirmation
```

The Store MVP should first reuse the already validated Solana program, Skill PDA, Offer and License model-00. No new on-chain program is required for the first Store proof.

## Day 7 closure

**Day 7 accepted by the user.** The embedded App now completes the Wallet -> Devnet -> licensed Skill discovery -> selection -> physical execution flow. Purchase remains separate by design.

Next development line: **JrSkill Store Web**, tracked in the separate `JrBot_Web` repository.

## Colosseum update — ready-to-publish draft

**Day 7 complete — JrSkill is now usable directly from the JrBot App.**

We moved the validated wallet/license flow from our development panel into the App served by the physical JrBot. Over HTTPS, the App authenticates a Phantom wallet, verifies Solana Devnet, discovers licensed Skills, selects `minimal_recipe_01` and executes it on the robot.

The validated App recovered the same 128-byte Skill payload and known hash, then finished physical execution with `result=ok`. Purchase is intentionally not part of the robot App: the next product layer is a separate JrSkill Store Web for discovering and buying Skills, while the JrBot App remains focused on using the licenses already owned by the wallet.

This keeps the architecture separated: Store Web for commerce, Solana for Skill/license state, and the JrBot App/Runtime API for safe physical execution.
