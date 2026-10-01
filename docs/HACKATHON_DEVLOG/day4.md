# 2026-10-01 — Day 4: Solana Skill to the existing JrBot executor

[Back to the development log index](../HACKATHON_DEVLOG.md)

**Stage 3 implemented; physical validation pending.** Work stays exclusively on `V1s-00`, after checking the current remote branch and Issue #33.

## Architecture and implementation

The PC panel now offers **Executar Skill da Devnet**, alongside the local-file baseline. A Python standard-library adapter reads the frozen Skill PDA from the official Devnet RPC at finalized commitment, validates its network/owner/Anchor layout/authority/schema/hash, and returns the original JSON text to the existing JavaScript executor.

```text
Solana Devnet Skill PDA
  → Python read-only validation
  → existing JrSkill Executor
  → existing local face_sequence Recipe
  → Runtime API 1.1 over Serial
  → physical JrBot (test pending)
```

Each run performs a new public read. Errors stop execution; there is no local Skill fallback. Logs identify `source=solana-devnet`, PDA, slot, payload length and hash. No wallet or private key is needed.

Panel version: `JRBOT-PANEL-V1S-SOLANA-02`. Firmware remains `JrBot_V1S_00`. The original JSON v1, local Recipe, contract and published PDA remain unchanged. Licensing and additional functions are outside this stage.

## Executed validation

- **5 Python tests passed**, including tampered account/network rejection, timeout without local fallback and HTTP success/failure through the panel wrapper chain.
- **2 JavaScript tests passed** with simulated Serial replies, including the expected face sequence and rejection before face commands on RPC/proof/capability failures.
- A **real read-only Devnet query** through the new Python adapter returned finalized slot `506320351`, **128 bytes**, `hash_verified: true`, `matches_checkpoint: true` and SHA-256 `416d6af34eada998a5f46595e0355a5bdfd7dba86faefe312dbc2afcd26d907f`.

These checks establish software integration and public retrieval. They do not establish physical execution from Solana. No firmware build, contract compilation/redeploy or blockchain write was performed.

## Installer configuration correction

The user's Windows build log failed in `espressif__esp_peer/src/dtls_srtp.c`, with missing MbedTLS DTLS/SRTP types/functions. Inspection found that `INSTALAR.bat` recognized only the historical `test/jrbot-v1s-00` branch name: the current `V1s-00` fell through to `build-runtime-api-v1-02` / `sdkconfig.runtime-api-v1-02`.

The installer now maps `V1s-00` explicitly to `build-v1s-00` and `sdkconfig.develop-v1.6.3`, matching the existing V1S configuration mapping. Validation of this change is static; a successful firmware rebuild is not claimed. Stage 3 needs only the updated PC panel: `INSTALAR.bat panel` updates the selected branch and opens the panel without building/flashing. Double-clicking the installer defaults to the complete build/flash path.

## Next checkpoint — user physical test

Follow the [Stage 3 architecture and physical test guide](../JRSKILL_SOLANA_STAGE3.md). Update the checkout used by the panel, restart it, connect the existing HW04 firmware by Serial and click **Executar Skill da Devnet**.

Success requires both the panel log (`source=solana-devnet`, verified hash and `result=ok`) and visually confirmed OLED transitions `happy → surprised → thinking → happy → neutral`. Save the full log, panel screenshot and robot video. A failed network read must not fall back to local execution.

**Day 4 remains open until the physical test is confirmed.**

---

[Previous: Day 3](day3.md)

## References

- [Issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33)
- [Stage 3 plan](../JRSKILL_SOLANA_STAGE3.md)
- [Python Devnet adapter](../../tools/jrbot_frontend/solana_skill.py)
