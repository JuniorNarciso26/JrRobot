# Stage 3 — Execute a Solana Skill on JrBot

**Date:** October 1, 2026. **Branch:** `V1s-00` only. [Issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33).

## Objective and architecture

Execute on the OLED the exact **128-byte JSON** published during Stage 2, but now retrieve it from its Solana PDA instead of reading a local Skill file.

```text
Execute Skill from Devnet button
  → GET /jrskill/skill-devnet on local Python server
  → getGenesisHash + getAccountInfo(finalized, base64) via Devnet RPC
  → verify Anchor/Borsh layout, authority, schema, SHA-256
  → pass recovered JSON to the existing JavaScript executor
  → resolve local face_sequence Recipe from the existing library
  → Runtime API 1.1 over Serial
  → physical OLED
```

The [`solana_skill.py` adapter](../../tools/jrbot_frontend/solana_skill.py) uses only Python's standard library, with no wallet, Node, Anchor, or npm dependency. **ESP32 does not connect directly to Solana**. The route returns the original UTF-8 text intact plus metadata kept outside the payload. The current executor interprets the JSON using existing functions. Local file execution remains available through a separate button.

This proof used only the frozen reference checkpoint:

- **Program ID:** `Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454`.
- **PDA:** `8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux`.
- **Authority:** `3Sce1sfA6q2m2mNr2VhyGfoTePa3vA9WjYYq2JYA5mij`.
- **SHA-256:** `416d6af34eada998a5f46595e0355a5bdfd7dba86faefe312dbc2afcd26d907f`.

Validation includes the full Devnet genesis hash, an existing non-executable account, expected owner, **589-byte** account layout, `account:Skill` discriminator, authority, schema 1, Borsh bounds, **128-byte** payload, stored/recomputed/checkpoint hashes, valid JSON v1, and a nonempty `run` field.

Calls remain restricted by the existing executor to `face`, `wait`, and `recipe`, with capability discovery and expression confirmation from the Runtime API.

Each click fetches network data anew. There is **no cache or fallback to a local Skill**. RPC/network/validation failures produce HTTP **502** and stop before any `face` command. RPC timeouts: 10 seconds for genesis and up to 15 seconds for the account; the panel's request timeout remains 35 seconds. Public RPC availability can vary.

Recipes remain local: only the top-level Skill JSON came from Solana. Contract, schema, original JSON, Recipe, and firmware were unchanged. Licensing was deferred to Stage 4.

## Panel identification and expected logs

Current reference panel: **`JRBOT-PANEL-V1S-SOLANA-03`**. Initial physical proof used version **02**; version **03** added the availability indicator.

Firmware: **`JrBot_V1S_00`**, without a new build.

Expected sample logs (slot varies):

```text
JR_SKILL_SOLANA fetch=started
JR_SKILL_SOLANA source=solana-devnet pda=8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux rpc_slot=... payload_bytes=128 payload_hash=416d6af34eada998a5f46595e0355a5bdfd7dba86faefe312dbc2afcd26d907f hash_verified=true matches_checkpoint=true
JR_SKILL_V1 start=v1 source=solana-devnet top_calls=4
JR_SKILL_V1 recipe_enter=face_sequence depth=1
JR_SKILL_V1 recipe_exit=face_sequence depth=1
JR_SKILL_V1 result=ok source=solana-devnet
```

`result=ok` indicates that the panel received successful Runtime API responses. A **physical** proof must also observe and record the OLED behavior.

## Automated and physical validation

- **5 Python tests passed**, covering exact payload/commitment, incorrect network, modified account metadata, timeout without fallback, and the actual `run_panel.py` HTTP wrapper path (success/HTTP 502).
- **2 Node tests passed**: a simulated Serial executor produced `happy → surprised → thinking → happy → neutral`, resolved the local Recipe, and did not load a local Skill file; bad RPC/proofs or unavailable capabilities sent no face commands.
- A live Python adapter query verified `finalized` commitment at slot **506320351**, **128 bytes**, and the correct hash/checkpoint.
- **Positive physical validation confirmed October 1, 2026.** The [original complete physical log](../06-colosseum-history/HACKATHON_DEVLOG/assets/day4/jrskill-devnet-physical-log.txt) records Devnet retrieval at slot **506352693**, validated 128-byte payload, `source=solana-devnet`, Runtime API responses for the five expressions, and `result=ok`. The maintainer confirmed the robot executed the sequence.

The local Recipe remained unchanged. No new contract deployment occurred. A separate firmware build attempt failed; the installer fix had only static verification, and should not be described as a successful firmware build.

### Reproduce software tests from the repository root

```bash
python -m unittest discover -s tools/jrbot_frontend/tests -p test_solana_skill.py -v
node --test tools/jrbot_frontend/tests/executor-devnet.test.mjs
python tools/jrbot_frontend/solana_skill.py
```

Node is required for the JavaScript development test only; the Python panel and live Skill query do not require it.

## Offline test — October 1, 2026

The [original offline-attempt log](../06-colosseum-history/HACKATHON_DEVLOG/assets/day4/jrskill-devnet-offline-log.txt) records `fetch=started` at **15:06:59**, followed by DNS error `[Errno 11001] getaddrinfo failed` and `result=error source=solana-devnet`.

There was **no Skill start, face command, Recipe, or `result=ok`** in that log. This verifies a software gate **before execution** with no successful fallback to the local Skill. That log alone did not include explicit visual confirmation that the OLED stayed unchanged or the first post-reconnection attempt. The earlier positive physical proof remained valid independently.

## Recovery and Devnet availability indicator

The maintainer supplied the [complete recovery log](../06-colosseum-history/HACKATHON_DEVLOG/assets/day4/jrskill-devnet-recovery-log.txt) and confirmed physical behavior, including local execution with and without Internet connectivity.

- **15:06:59:** Devnet attempt failed.
- **15:09:43:** new attempt retrieved 128 bytes at slot **506359801** and completed with `result=ok source=solana-devnet` at **15:09:48**.
- **15:10:07:** local execution completed successfully.
- **15:10:09 / 15:10:18:** further Devnet attempts failed.
- **15:10:23:** the Skill was retrieved again, but the provided log ended before final completion was recorded.

The complete 15:09:43 recovery demonstrates successful return to online execution.

In version **03**, `/jrskill/devnet-status` calls `getGenesisHash` with a **5-second** timeout and validates the cluster. The panel checks this on opening, every **30 seconds** when idle, and on **Verify Devnet**.

Solana connectivity is displayed **separately from Serial**: RPC/network failure does not mean the USB-connected robot is disconnected. Local Skills and Serial controls remain available. The Devnet button is enabled only after a successful check, and every execution still refetches and revalidates the payload. Manual/periodic checks can restore availability when RPC returns.

For version 03, **6 Python and 3 JavaScript tests passed**, including availability transitions and local offline execution; a live check confirmed Devnet connectivity.

The maintainer confirmed the improvement on October 1 and supplied the [panel/robot status log](../06-colosseum-history/HACKATHON_DEVLOG/assets/day4/jrskill-panel-status-log.txt): COM7, `JrBot_V1S_00`, HW04 and OLED available. That file **did not** record Devnet indicator transitions; UI confirmation came from the user's observation, not that log.

No changes to firmware, JSON, or contract occurred.

## Original physical test plan

1. Update the **checkout actually used by the panel** on `V1s-00`, using `git pull --ff-only origin V1s-00`.
2. Close the old panel to release port 8765 and Serial. Start the updated panel normally or use `python tools/jrbot_frontend/run_panel.py` from the repository root.
3. Use the validated Windows Serial environment for the panel's `COM` port list. WSL may run Python queries and tests, but this panel does not enumerate `/dev/tty*`.
4. Confirm `JRBOT-PANEL-V1S-SOLANA-03`, wait for **Solana Devnet: available**, connect the robot's COM port, and check firmware/HW04/OLED status.
5. Clear the log to distinguish this from local execution, then click **Execute Skill from Devnet**.
6. Observe `happy → surprised → thinking → happy → neutral` on the OLED. Check `solana-devnet` source, PDA/hash, local Recipe, and `result=ok`.
7. Download the text log and capture a panel screenshot/OLED video. Record results in Issue #33 and the Day 4 log.
8. For the negative test, keep Serial connected but disconnect the PC from the Internet, then click **Verify Devnet** or wait for the periodic check. The Devnet indicator/button should become unavailable while local execution remains available. If an execution begins just before the outage is detected, it must fail without `start`/`result=ok` or a new face command. Restore Internet, verify again, and repeat the positive flow. Never silently fall back to local Skill execution.

No ESP32 reflash or `anchor build/deploy` is required: this uses the previously published program and PDA.

## References

- [Solana RPC getAccountInfo](https://solana.com/docs/rpc/http/getaccountinfo).
- [Solana RPC getGenesisHash](https://solana.com/docs/rpc/http/getgenesishash).
- [Stage 2 — Skill PDA](JRSKILL_SOLANA_STAGE2.md).
- [Colosseum Day 4](../06-colosseum-history/HACKATHON_DEVLOG/day4.md).
