# Discovering Wallet-Licensed Skills — SKILLS-14

**Date:** October 5, 2026. **Branch:** `V1s-00` only. **Panel:** `JRBOT-PANEL-V1S-SKILLS-14`. [Issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33).

**User acceptance (Day 6 closing, Log 81):** automatic/manual discovery returned one licensed Skill, and licensed execution completed on the robot. Details follow. No new purchase or program deployment occurred in this checkpoint.

## Scope

The existing layout was preserved. The original **My Test Skill** area was renamed **My Skills**, with a **Find My Skills** button, search status, and a Skill list.

Discovery runs automatically after authentication, on manual refresh, and after purchasing a new license. Changing or disconnecting wallets or losing authentication clears the list and discards delayed responses. An RPC failure must be reported as **unconfirmed search**, never as confirmation that the wallet owns no Skills.

This work added discovery **without general execution selection**. Purchasing and authorized execution continued using the existing `minimal_recipe_01` checkpoint. Other valid licensed Skills could be listed but were marked not yet executable. Discovery did not issue a permit, send physical commands, or replace the per-command license checks of the existing executor. No new contract, firmware, JSON schema, Recipe, or deployment.

## Architecture

`GET /jrskill/wallet/skills` uses the wallet bound to the authenticated session; it does not accept a client-supplied buyer in the URL. Authentication and buyer identity are checked both before and after the request.

After verifying the Devnet genesis hash, `getProgramAccounts` with `finalized/withContext` discovers **137-byte License** accounts using the discriminator and buyer at **offset 9**. The server validates owner, discriminator, version 00, buyer, derived License PDA, derived Offer PDA, amounts paid, and duplicates.

Linked Skill/Offer accounts are loaded using `getMultipleAccounts` at `finalized` commitment with `minContextSlot`. Checks include owner, account size, schema v1, SHA-256 payload hash, PDA derived from `[skill, authority, hash]`, JSON v1 validity, and Skill/Offer/creator/price relationships.

There is no cache or local fallback. The pilot limits each search to **32 licenses** and reports an error if that limit is exceeded or the RPC response is larger than **64 KiB**. It must not silently truncate results or display an empty list. Pagination and external indexing are outside this checkpoint.

The current on-chain schema has **no name or description fields**. The known reference checkpoint is labeled `minimal_recipe_01`; other results display `Skill` plus an address prefix. Addresses and hashes appear under **View Skill and License**. Do not invent commercial titles from payload data.

Primary API reference: [Solana getProgramAccounts](https://solana.com/docs/rpc/http/getprogramaccounts).

## Verification performed

- **39 Python tests passed**, covering two Skills from different creators licensed to one buyer, filters, PDA/hash/owner relationships, duplicates and limits, wrong network, RPC failures, empty results, and a session change during HTTP.
- **29 JavaScript tests passed**: 24 wallet tests (including discovery, refresh, authentication, delayed responses) and five existing executor tests. The bundle was rebuilt.
- **Live Devnet read, without transaction:** licensed wallet `6rinyiBQyS4RxLeuX62yJqxgwiJqWXiyAbxXSS5LX85R` returned `minimal_recipe_01`, License PDA `7tPf4YSd7P6PzBkmseW5FrwnG8P238gZTVG8Rj2v9v45`, verified hash, `finalized` slot **507816052**. Unlicensed wallet `8zQwpe3qVBzoPMasLQSbeagtmKzqL1JLG4iEApznUySo` returned an empty list at slot **507816055**.
- A local browser check confirmed the prior layout, SKILLS-14, My Skills area, and blocked discovery without a connected wallet. With neither Phantom nor a physical robot attached during this particular check, that check did **not** prove extension or hardware behavior. No contract or firmware build/deploy occurred.

## User result — Log 81

A licensed wallet authenticated at **15:09:42**. Automatic discovery at **15:09:44** returned `count=1` at slot **507817275**. Manual refresh at **15:09:47** returned `count=1` at slot **507817287**. Duplicate frontend/backend log lines represent **one result**, not extra licenses.

Authorization at **15:09:57** read the original **128-byte** Skill with verified hash at slot **507817329**. `capabilities` and OLED face commands were acknowledged by `JrBot_V1S_00` / HW04. The sequence `happy -> surprised -> thinking -> happy -> neutral` ended at **15:10:06** with `result=ok` and `source=solana-devnet`. The user accepted the test as complete.

[Log 81 excerpt](../06-colosseum-history/HACKATHON_DEVLOG/assets/day6/panel-log-81-discovery-execution-excerpt.txt).

This log supports discovery by the licensed wallet and reference-Skill execution on hardware. It does **not** prove discovery of multiple Skills, switching to an unlicensed wallet, or RPC-error handling on hardware. Those cases rely on earlier software/RPC verification. This was a documentation checkpoint without new code, build, deployment, or automated test.

## Original panel test procedure — results documented above

1. Stop the panel server. Confirm `V1s-00` in the Windows checkout, run `git pull --ff-only origin V1s-00`, launch `PAINEL.bat`, refresh with Ctrl+F5, and confirm SKILLS-14. No npm/Anchor, firmware flash, contract deployment, or repeat purchase is needed.
2. Connect/authenticate the licensed buyer wallet. **My Skills** should show `minimal_recipe_01`. **Find My Skills** repeats discovery. **View Skill and License** shows PDAs.
3. Switch to a wallet without a license and authenticate. The old list must be cleared; successful discovery should report no licenses for this program on Devnet. A network error must instead report **unconfirmed search**.
4. The original Skill's existing execution button still requires the licensed wallet and Serial. Discovery must not execute actions automatically.

**Next proposed step at the time:** generic selection/execution of discovered Skills and integration into the embedded robot App. The web Store remains the marketplace; the panel remains a development tool.
