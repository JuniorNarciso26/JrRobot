# JrSkill — Embedded App / My Skills

**Branch:** `V1s-00`  
**Reference candidate:** `JrBot_V1S_APP_03`  
**Last documented physical validation:** October 7, 2026

## Architecture decision

Instead of building a separate application from scratch, the existing local JrBot App receives a **My Skills** board. The development panel remains separate as a diagnostic and laboratory tool.

## My Skills board

The embedded App supports:

- Connecting a wallet.
- Authenticating a wallet by signed message.
- Checking Devnet and wallet balance.
- Discovering licensed Skills.
- Selecting a Skill.
- Executing an authorized Skill on JrBot.
- Disconnecting the wallet.

Discovery reads the deployed JrSkill Devnet program and validates the program owner, Skill and License discriminators, buyer, payload schema, and SHA-256 hash. The selected license account is read again before each physical action.

## Purchases stay in the web Store

The firmware App does **not** quote prices, accept purchase costs, offer a Buy button, sign a purchase transaction, or host a marketplace. Purchasing takes place through the JrSkill web Store.

```text
Web Store
  -> Purchase Skill
  -> License PDA
  -> JrBot App / My Skills
  -> Select Skill
  -> Execute
```

## Solana interaction location

The ESP32 itself does **not** become a full Solana client:

```text
Browser
  +-> Phantom
  +-> Solana Devnet RPC
  +-> Validate License/Skill
  +-> JrBot /cmd
        -> Runtime API
        -> Hardware
```

The firmware serves the App's JavaScript. Browser-side JavaScript handles HTTPS communication with Solana Devnet.

## Executor

The initial validated contract uses `face`, `wait`, and a local `face_sequence` recipe. The JrSkill JSON v1 schema and on-chain program were not changed in this stage.

## Earlier App builds

Because firmware was changed, previous installed builds could not be reused.

- **`JrBot_V1S_APP_01`:** installed with the correct version displayed, but the My Skills board failed to appear.
- **`JrBot_V1S_APP_02`:** corrected delivery of the embedded JavaScript. Its build directory was `build-v1s-app-02`.

### APP_02 JavaScript fix

The `/jrskill-app.js` handler used `EMBED_TXTFILES`, which embeds a null-terminated string. In APP_01, the handler included the trailing NUL byte in its HTTP response. That byte made the JavaScript invalid to the browser parser.

APP_02 served the string without the terminating byte:

```c
httpd_resp_send(req, jrskill_app_js_start, HTTPD_RESP_USE_STRLEN);
```

APP_02 was built, installed, and physically tested on **October 6, 2026**. In the local App over **HTTPS**, the maintainer confirmed the My Skills board, authenticated wallet, Devnet balance, licensed discovery of `minimal_recipe_01`, a **128-byte** payload, hash beginning `416d6af34ead...`, Skill selection, and execution with **`result=ok`**.

The preceding HTTP-only access attempt was blocked by browser security restrictions. Wallet/Devnet functionality for this stage must be accessed through `https://<JRBOT_IP>/`.

### Initial APP_02 test procedure

1. Update `V1s-00`.
2. Run `INSTALAR.bat`.
3. Confirm the installed build identifier.
4. Open the JrBot HTTPS App in a browser supporting Phantom.
5. Find **My Skills**.
6. Connect and authenticate the wallet.
7. Verify Devnet and discover purchased Skills.
8. Select `minimal_recipe_01` and run it.
9. Physically verify `happy -> surprised -> thinking -> happy -> neutral`.
10. Confirm the App contains no purchase workflow.

## APP_03 / SKILLS-15 panel — Expressions and generic selection

Physical testing of `jrteste` exposed an App allowlist limited to four OLED expressions. APP_03 expanded the App and panel authorizer to the OLED module's **16 expressions**, including `sad` and `worried`; the firmware accepts `battery_low` as the canonical name.

The App validates the complete JSON document, including recipes, before its first Runtime API command. The panel can select any licensed JSON v1 Skill discovered on Devnet. Reads verify account owner, discriminator, schema, payload size, hash, and the author/hash-derived PDA.

Execution permission is bound to the authenticated session, buyer, and chosen PDA. The license is read with `finalized` commitment before each command. The reference Skill is a fallback for legacy calls without an explicit Skill parameter; the UI requires selection. Other schemas remain rejected, and purchases stay in the Store.

### APP_03 test procedure

1. Update `V1s-00` and install `JrBot_V1S_APP_03` using `INSTALAR.bat`.
2. Confirm `APP_03` firmware and panel `JRBOT-PANEL-V1S-SKILLS-15`.
3. Reload the App, authenticate the buyer wallet, and discover both Skills.
4. Select Skill `8AgpL6dM` and execute `happy → sad → thinking → neutral → worried`.
5. In the panel, authenticate, discover Skills, select the same PDA, and execute.
6. Repeat with `minimal_recipe_01` to verify compatibility.

### Automated validation and build record

APP_03 compiled on **October 7, 2026** with ESP-IDF 5.5.5, ESP32-S3, and the HW04 `sdkconfig.develop-v1.6.3` configuration. The application binary was `0x217740` bytes with 58% of the app partition free.

Automated tests included **43 Python tests, 33 JavaScript tests, and six panel-policy checks**. A live Devnet read verified the `jrteste` payload (**170 bytes**), hash, and five expressions. That read-only verification sent **no physical command**.

### Final physical validation

**APP_03 physical testing passed on October 7, 2026.** The maintainer confirmed `jrteste` completion in both App and panel. The App screenshot showed `result=ok`, **170 bytes**, and a hash beginning `68688b90b773...`. No new panel log was provided with that confirmation.

The earlier "physical testing pending" entries in the development record describe intermediate states before the October 7 confirmation; they do not override the final validation.
