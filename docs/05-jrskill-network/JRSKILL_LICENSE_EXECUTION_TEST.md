# Stage 4E — Authorize Devnet Execution Through License PDA

**Date:** October 5, 2026. **Branch:** `V1s-00` only. **Panel:** `JRBOT-PANEL-V1S-LICENSE-12`. [Issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33).

**User tests (logs 79 and 80) recorded:** blocking execution without a license, licensed execution, interruption upon wallet disconnection, communication failure, and successful recovery. An independent RPC check confirmed the state of both wallets in checkpoint 79. The user accepted these tests; the documented evidence limits remain important. This checkpoint did not change or redeploy the Solana program, firmware, JSON v1, or Recipe.

## Architecture

- An authenticated wallet enables a Devnet execution attempt. `POST /jrskill/wallet/execution/start` validates origin, cookies, and session; reads the license at `finalized` commitment; and loads the on-chain Skill after validating hash, schema, and authority. Missing license or RPC access blocks execution **before physical commands**.
- The `wallet_execution.py` backend issues a random **single-execution permit** bound to session, origin, and buyer, with a **120-second TTL** and capacity of up to **64 entries**. A new execution invalidates any preceding permit in the same session. Permits stay in memory and are not written to text logs.
- The server derives the authorized sequence from the verified Skill and the local `face_sequence` Recipe: `capabilities`, `happy`, `surprised`, `thinking`, `happy`, `neutral`. Waits remain in the local executor. The `/execution/send` endpoint accepts only the **next** command over Serial, and rechecks the finalized license **before every command**. There is no cached ownership or local authorization fallback.
- After every RPC response, session, buyer, expiration, and permit are revalidated. A command is consumed before I/O, with no automatic retry. Permits are removed on completion or failure. Replay, out-of-order commands, conflicting sessions/origins, and concurrent execution are rejected.
- The frontend tracks account, provider, and authentication revisions before and after asynchronous waits and commands. Switching wallets, disconnecting, or reauthenticating interrupts the previous execution. A delayed response must never authorize a newly selected wallet.
- `JR_SKILL_AUTHORIZATION` logs `allowed` with buyer/license, `command_sent` for each authorized command, or `blocked` with a reason. `result=ok` appears only after the full sequence completes without invalidation.

## Scope and limitations

This authorization protects **Execute Skill from Devnet** in the development panel/backend. The on-chain JSON and read-only endpoints remain public. Manual controls, local file tests, and `/send` remain license-free development tools, allowing the PC operator to control their own robot. This is **not firmware DRM** and does not prevent a modified client.

An RPC read per command can add latency; `wait=500` does not guarantee precisely 500 ms between visual changes. Interruptions cannot undo commands already sent. The previous face may remain displayed. Do not automatically send `neutral` when authorization is denied. An in-flight physical command cannot be recalled, but subsequent commands are blocked.

## Software verification

Passed **34 Python tests, 21 wallet JavaScript tests, and 5 executor JavaScript tests**. Coverage included a complete sequence; license absence; authentication; expiration; wallet changes during RPC; license/RPC loss; permit replacement; replay; and out-of-order commands.

An HTTP test with actual disposable-wallet message authentication and mocked RPC/Serial confirmed 401 without session, rejection without license, 403 for wrong origin, valid command execution, and logout blocking the next action.

The simulated executor verified that negative cases sent **no OLED commands**, and that an account/RPC change after the first face interrupted subsequent commands. The panel bundle was rebuilt. **Those automated tests were not physical validation**.

## Original physical test procedure — results documented below

1. Stop the old server, update the Windows `V1s-00` checkout (`git pull --ff-only origin V1s-00`), launch `PAINEL.bat`, press Ctrl+F5, and confirm panel `LICENSE-12`. Do not reflash firmware or redeploy the program.
2. Connect Serial; confirm HW04/OLED and Devnet status. Without wallet authentication, Devnet execution must be blocked while local testing remains available.
3. Authenticate a disposable wallet **without a license**; querying the license should return absent. Devnet execution should log `JR_SKILL_AUTHORIZATION result=blocked` without changing the robot face. Do not buy a license with this wallet during the negative test.
4. Reconnect licensed wallet `6rinyiBQyS4RxLeuX62yJqxgwiJqWXiyAbxXSS5LX85R`. Expected: `allowed`, six `command_sent` records (capabilities plus five face commands), `source=solana-devnet`, `result=ok`, and a visually confirmed expression sequence.
5. Disconnect or change the wallet during the sequence. Remaining commands should stop, with no `result=ok`; already-sent faces may remain. Reauthenticate for a new attempt.
6. Disable the PC's Internet connectivity; Devnet execution must block while local tests still work. Restore connectivity and recheck Devnet. The robot's own Wi-Fi does not replace the computer's RPC connection.
7. Record text logs and visual observations separately. Do not modify parameters after an error without reviewing its cause.

## Result — Log 79: wallets without and with licenses

Unlicensed wallet `8zQwpe3qVBzoPMasLQSbeagtmKzqL1JLG4iEApznUySo` connected at **12:55:43**, authenticated at **12:55:49**, and queried at **12:55:57**. The result was `owned=false`; derived license PDA `FSmHntJybFcGbhZWLBv4Us4AjSEuG92RYEQAenM1jEMT` was absent.

Execution attempts at **12:56:01**, **12:56:12**, and **12:56:33** were blocked with `execution_license_absent`. No `command_sent` from Devnet execution was recorded for this wallet. A zero balance was not the recorded reason: **the license was missing**.

The user disconnected at **12:56:40**, connected licensed wallet `6rinyiBQyS4RxLeuX62yJqxgwiJqWXiyAbxXSS5LX85R` at **12:56:50**, and authenticated at **12:56:55**. Authorization was `allowed` at **12:56:59**, for license `7tPf4YSd7P6PzBkmseW5FrwnG8P238gZTVG8Rj2v9v45`.

The Devnet Skill was read at slot **507783945**: **128 bytes**, hash verified. Six commands were sent, completing `happy -> surprised -> thinking -> happy -> neutral` with `result=ok`, `source=solana-devnet` at **12:57:10**. The user confirmed success on the physical robot.

Independent RPC confirmed `owned=false` for the new wallet and `owned=true` for the original, with finalized commitment. [Public wallet-state proof](../06-colosseum-history/HACKATHON_DEVLOG/assets/day6/license-gate-wallets-proof.json) · [Log 79 excerpt](../06-colosseum-history/HACKATHON_DEVLOG/assets/day6/panel-log-79-license-excerpt.txt).

**Evidence boundary:** the wallet switch happened **between** attempts, not during an active sequence. Log 79 alone does not prove live wallet-switch interruption or offline behavior. These records changed documentation only; no code, build, or deployment was performed.

## Additional result — interruption and recovery, Log 80

The user tested the physical robot and accepted the results. The table describes what the text log demonstrates, without implying further simulations or retesting.

| Time | Recorded evidence | Outcome |
| --- | --- | --- |
| 13:04:56 | Licensed wallet; Devnet payload verified | Completed, `result=ok` |
| 13:05:01–13:05:02 | Wallet disconnected during a new sequence | `execution_auth_required`; no completed `result=ok` |
| 13:05:20–13:05:21 | Wallet disconnected during a `source=local-file` execution | Local test completed; expected development-mode behavior |
| 13:05:41 | Disconnection during another Devnet sequence | Frontend guard stopped subsequent commands |
| 13:08:10 | Reauthentication and new authorization | Devnet execution completed, `result=ok` |
| 13:08:19 | Connection event from same address during execution | Wallet state invalidated the sequence; no completed `result=ok` |
| 13:09:17 | Unlicensed wallet queried; `owned=false` | `execution_license_absent`; no authorized commands |
| 13:09:52 | Return to licensed wallet | New Devnet sequence completed, `result=ok` |
| 13:10:05 | I/O failure: `Remote end closed connection without response` | Authorization blocked, sequence incomplete |
| 13:10:25–13:10:37 | Reauthentication and retry after failure | Payload verified; sequence completed, `result=ok` |

[Log 80 excerpt](../06-colosseum-history/HACKATHON_DEVLOG/assets/day6/panel-log-80-interruption-excerpt.txt) preserves Skill and wallet-state events; other lines were omitted.

The log supports interruption on wallet invalidation/disconnection or communication error, followed by recovery. It does **not** independently establish the physical root cause of the I/O failure (for example, an Internet outage), nor a switch to a **different** address mid-sequence.

Already-sent commands cannot be undone. License-free local tests deliberately continue; the Devnet panel flow is what this mechanism protects, not the underlying firmware from all possible clients.

**Checkpoint accepted.** This documentation revision does not represent new automated tests, build, deployment, payment, or firmware/contract/JSON/Recipe modification.
