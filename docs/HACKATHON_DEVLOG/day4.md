# 2026-10-01 — Day 4: Solana Skill to the existing JrBot executor

[Back to the development log index](../HACKATHON_DEVLOG.md)

**Stage 3 positive path physically validated by the user on 2026-10-01.** Work stays exclusively on `V1s-00`, after checking the current remote branch and Issue #33.

## Architecture and implementation

The PC panel now offers **Executar Skill da Devnet**, alongside the local-file baseline. A Python standard-library adapter reads the frozen Skill PDA from the official Devnet RPC at finalized commitment, validates its network/owner/Anchor layout/authority/schema/hash, and returns the original JSON text to the existing JavaScript executor.

```text
Solana Devnet Skill PDA
  → Python read-only validation
  → existing JrSkill Executor
  → existing local face_sequence Recipe
  → Runtime API 1.1 over Serial
  → physical JrBot (user-confirmed execution)
```

Each run performs a new public read. Errors stop execution; there is no local Skill fallback. Logs identify `source=solana-devnet`, PDA, slot, payload length and hash. No wallet or private key is needed.

Panel version: `JRBOT-PANEL-V1S-SOLANA-02`. Firmware remains `JrBot_V1S_00`. The original JSON v1, local Recipe, contract and published PDA remain unchanged. Licensing and additional functions are outside this stage.

## Executed validation

- **5 Python tests passed**, including tampered account/network rejection, timeout without local fallback and HTTP success/failure through the panel wrapper chain.
- **2 JavaScript tests passed** with simulated Serial replies, including the expected face sequence and rejection before face commands on RPC/proof/capability failures.
- A **real read-only Devnet query** through the new Python adapter returned finalized slot `506320351`, **128 bytes**, `hash_verified: true`, `matches_checkpoint: true` and SHA-256 `416d6af34eada998a5f46595e0355a5bdfd7dba86faefe312dbc2afcd26d907f`.

These initial agent checks established software integration and public retrieval; physical execution was still pending at that checkpoint. No firmware build, contract compilation/redeploy or blockchain write was part of those checks. The subsequent user physical result is recorded below.

## Installer configuration correction

The user's Windows build log failed in `espressif__esp_peer/src/dtls_srtp.c`, with missing MbedTLS DTLS/SRTP types/functions. Inspection found that `INSTALAR.bat` recognized only the historical `test/jrbot-v1s-00` branch name: the current `V1s-00` fell through to `build-runtime-api-v1-02` / `sdkconfig.runtime-api-v1-02`.

The installer now maps `V1s-00` explicitly to `build-v1s-00` and `sdkconfig.develop-v1.6.3`, matching the existing V1S configuration mapping. Validation of this change is static; a successful firmware rebuild is not claimed. Stage 3 needs only the updated PC panel: `INSTALAR.bat panel` updates the selected branch and opens the panel without building/flashing. Double-clicking the installer defaults to the complete build/flash path.

## User physical validation — 2026-10-01

The user supplied [the complete panel log](assets/day4/jrskill-devnet-physical-log.txt) and confirmed that the physical robot ran using the Skill from Solana. The log records the following sequence between **14:42:04 and 14:42:08** (panel clock; no timezone is embedded in the file):

```text
[14:42:04] JR_SKILL_SOLANA fetch=started
[14:42:05] JR_SKILL_SOLANA source=solana-devnet pda=8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux rpc_slot=506352693 payload_bytes=128 payload_hash=416d6af34eada998a5f46595e0355a5bdfd7dba86faefe312dbc2afcd26d907f hash_verified=true matches_checkpoint=true
[14:42:05] JR_SKILL_V1 start=v1 source=solana-devnet top_calls=4
[14:42:06] JR_SKILL_V1 recipe_enter=face_sequence depth=1
[14:42:08] JR_SKILL_V1 recipe_exit=face_sequence depth=1
[14:42:08] JR_SKILL_V1 result=ok source=solana-devnet
```

Runtime API replies identify firmware **`JrBot_V1S_00`**, hardware **`JRBOT-HW-04`** and API **1.1**, and acknowledge every expression in order: **happy → surprised → thinking → happy → neutral**. Four top-level calls ran, including the local Recipe with six nested calls.

Evidence levels are distinct: the log proves the validated Devnet source and successful Runtime API replies; the user confirms the physical robot execution. The original log is preserved byte for byte, SHA-256 `666da90574c7d2272328b3a105aee8f3c6e60f7e44b35feee07eeb20718b4826`. No new Day 4 screenshot/video was supplied with this result.

**The Stage 3 positive path is now validated: Solana → existing executor → Runtime API → physical JrBot.** Only the main Skill was retrieved from Solana; `face_sequence` remains in the local Recipe library. This result does not validate licensing, publication of Recipes, the installer firmware rebuild or the physical offline/failure test.

## User offline test — 2026-10-01

The user supplied [the complete disconnected-Internet test log](assets/day4/jrskill-devnet-offline-log.txt). At **15:06:59** (panel clock), the request failed during DNS resolution:

```text
[15:06:59] JR_SKILL_SOLANA fetch=started
[15:06:59] JR_SKILL_V1 result=error source=solana-devnet detail=Leitura Devnet recusada: <urlopen error [Errno 11001] getaddrinfo failed>
[15:06:59] ERRO: Leitura Devnet recusada: <urlopen error [Errno 11001] getaddrinfo failed>
[15:06:59] JR_SKILL_SOLANA result=error detail=<urlopen error [Errno 11001] getaddrinfo failed>
```

The supplied log contains no Skill `start`, Runtime API/face command, Recipe execution or `result=ok`. It confirms that this offline attempt was blocked before execution, without a successful local fallback. The original file is preserved without editing.

This file does not contain the subsequent online retry or a visual observation of the OLED. The earlier positive physical run remains independently validated; recovery after this particular disconnection and explicit visual confirmation that the OLED stayed unchanged are still awaiting user evidence.

## Full offline/online recovery test and local baseline

The user supplied [the full recovery log](assets/day4/jrskill-devnet-recovery-log.txt) and confirmed that the physical test worked as expected, including local execution with/without Internet and recovery when Internet returned.

- **15:06:59:** Devnet request rejected by DNS failure, before Skill execution.
- **15:09:44:** Devnet read succeeds at slot `506359801`, with 128 bytes and verified checkpoint hash; **15:09:48:** `result=ok source=solana-devnet` after all face acknowledgements.
- **15:10:04–15:10:07:** explicit local-file execution completes with `result=ok source=local-file`; the user confirms the local path works without Internet.
- **15:10:09 and 15:10:18:** Devnet requests fail with DNS errors; neither attempt starts the Skill.
- **15:10:25:** another valid Devnet read succeeds at slot `506359973` and execution restarts. The supplied file ends at the final `face neutral` call at **15:10:28**, before its reply/result; the earlier complete recovery establishes the successful return to online execution.

The original log is preserved byte for byte, SHA-256 `fa26db912f6ab73634215bce3e369e7ee3f3284511ea36662d0c68d225f21c5d`. Network state is inferred from failed/successful RPC calls and the user's test description; local execution is an explicit separate path, not a fallback from a failed Devnet call.

## Devnet availability indicator

Following this physical result, the panel was updated to **`JRBOT-PANEL-V1S-SOLANA-03`**. It checks access to the fixed official Devnet RPC on opening, then every 30 seconds while the panel is idle, with a manual **Verificar Devnet** button. The probe uses `getGenesisHash`, validates the Devnet identity and does not read/execute/cache a Skill.

The panel shows **Solana Devnet: verificando / disponivel / indisponivel** independently of the robot Serial connection. Only the Devnet execution button is gated by this status; local Skill/manual Serial controls remain available. A new Skill execution still performs full validation and a fresh read, and a failure updates the availability indicator. A successful probe cannot guarantee the subsequent read will succeed.

Validation of this UI update: **6 Python + 3 JavaScript tests passed**, including availability transitions, offline local execution and HTTP status routes. The live probe returned `available: true, cluster: devnet`. These software checks do not claim user physical validation of panel version 03 yet. Firmware, contract and JSON remain unchanged.

## Reproduction and remaining evidence

Follow the [Stage 3 architecture and physical test guide](../JRSKILL_SOLANA_STAGE3.md). Update the checkout used by the panel, restart it, connect the existing HW04 firmware by Serial and click **Executar Skill da Devnet**.

Success requires both the panel log (`source=solana-devnet`, verified hash and `result=ok`) and visually confirmed OLED transitions `happy → surprised → thinking → happy → neutral`. Save the full log, panel screenshot and robot video. A failed network read must not fall back to local execution.

The Stage 3 execution path, offline block, local independence and online recovery are confirmed by the logs and user physical report. Panel version 03's new availability indicator awaits user confirmation. A panel screenshot/robot video can complement the preserved logs when available. Licensing remains a separate next stage.

---

[Previous: Day 3](day3.md)

## References

- [Issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33)
- [Stage 3 plan](../JRSKILL_SOLANA_STAGE3.md)
- [Python Devnet adapter](../../tools/jrbot_frontend/solana_skill.py)
