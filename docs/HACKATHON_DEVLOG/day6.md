# Day 6 — first license purchase on Solana Devnet

Date: 2026-10-05. Branch: `V1s-00`. Model 00, CLI proof using test SOL.

The user funded a separate buyer wallet, reviewed a read-only quote and explicitly sent the purchase. The 1 test SOL price was paid in the same transaction that created the buyer's License PDA.

- Buyer: `E5F1ztxZwgogcwWaryB6uCLXp3iZ2YA6MBzjwv6R51dE`.
- Skill: `8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux`.
- License: [5wjuV3W2fJo9o8U5oqfjcnbUULGxKqRxLXK9AgUvqADR](https://explorer.solana.com/address/5wjuV3W2fJo9o8U5oqfjcnbUULGxKqRxLXK9AgUvqADR?cluster=devnet).
- Transaction: [view purchase](https://explorer.solana.com/tx/4dwpsPA6GrZNhLuJhmdWQPJSv1r3QLnpMnxGrY4PvdQj2AAEzs2aeWu9YrBS7Kut5rhDgMenq9U7wiEd1GnMLKiY?cluster=devnet).

Independent RPC inspection confirmed finalized transaction slot **507724452**, no transaction error, license owner/type/identity/model/payment fields, and the following exact balance movements:

| Movement | Test SOL |
| --- | ---: |
| Creator received | 0.5 |
| JrBot treasury received | 0.5 |
| License account deposit | 0.0013462 |
| Network fee | 0.000005 |
| Total buyer debit | 1.0013512 |

The buyer moved from 5 to 3.9986488 test SOL in this transaction. Full account pre/post balances: [public RPC proof](assets/day6/license-purchase-proof.json). No private keys or seed phrases are included.

## Buyer A repeat and independent buyer B

The user repeated A's --send command; the client returned `already_exists: true` with the same License PDA. This is the client check that skips sending another transaction, not an on-chain duplicate-rejection test. The earlier local-validator tests cover contract rejection and rollback.

Buyer B `Dyf2qbLSn7pW7khP4z4xWykDr7Kuw5uMqmxzMFRuU6vs` was read before purchase: `exists: false`. B then reviewed a quote, explicitly purchased and read the new license: `exists: true`, model 0, price 1000000000 lamports, creator/treasury 500000000 each.

| Buyer | License PDA | Skill |
| --- | --- | --- |
| A | `5wjuV3W2fJo9o8U5oqfjcnbUULGxKqRxLXK9AgUvqADR` | `8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux` |
| B | `CdvpW2VoTK4E1H39d688g9mW9vFtLwasqXrdiVuModLo` | Same Skill |

B transaction: [view purchase](https://explorer.solana.com/tx/oZkPZsQXtoYfC5FhjHjKS9zoyhtGsVkERdZaa6HKuBB4Y2x4Fuqif2TEa1ZErUmEsN8HZb5N3gmjk15nLNNd9jD?cluster=devnet). Independent RPC inspection confirms **finalized slot 507731090**, no error, valid buyer/Skill/license/terms and the same exact payment/rent/fee amounts as A. B's debit was 1.0013512 test SOL; the creator and treasury each received another 0.5. [B public RPC proof](assets/day6/license-purchase-b-proof.json).

The positive A/B proof is complete: one shared Skill, independent paid licenses. Remaining work: negative contract cases on Devnet if required, Phantom purchase, licensed Skill listing and authorization before physical execution. No firmware, JSON/Recipe or program code changed in this documentation checkpoint.

Detailed terms/results: [model 00 checkpoint](../JRSKILL_LICENSE_MODEL_00_TEST.md). History: [commercial issue #46](https://github.com/JuniorNarciso26/JrRobot/issues/46), [technical issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33).

## Browser purchase implementation — Stage 4D

Panel **JRBOT-PANEL-V1S-PURCHASE-08** now reads the connected authenticated wallet's license for the known test Skill, presents an unsigned purchase quote and asks for explicit acceptance before Wallet Standard signing on solana:devnet. The backend verifies the exact signed quote, session, expiry and blockhash before relay. A fresh finalized license read is required to show ownership. Ambiguous results retain the signature and prevent automatic retries.

Validation: **23 Python tests and 18 JavaScript tests passed**; Python/Anchor IDL transaction bytes matched (417 bytes); a Chrome flow passed with a simulated wallet/RPC and real test signatures. The existing A license was also read through the new decoder using real Devnet RPC. These are software tests and a read-only check: no new purchase/deploy on Devnet, real Phantom extension test or physical test occurred. Firmware, program and frozen JSON remain unchanged.

Next: test the purchase with the real Phantom extension, following [Stage 4D instructions](../JRSKILL_WALLET_PURCHASE_TEST.md). General Skill discovery and license authorization before robot execution remain future work.

## Browser purchase diagnostics — PURCHASE-09

The user's panel log (75) records quotes, a 5 test SOL balance and an absent license, without a successful relay signature. An independent RPC check also found no license or transaction history for its address. The original failure reason was hidden by generic HTTP/RPC handling, so it remains undetermined. A later license check incorrectly said "purchase sent" for an uncertain attempt.

PURCHASE-09 corrects that message and records signing, client HTTP, server validation and RPC relay stages with bounded error details, HTTP status and public signature when available. Unknown results retain the no-retry guard; no payment success is inferred. **24 Python and 20 JS tests passed**, and the bundle was rebuilt. No new Devnet purchase/deploy or physical test occurred. The next real Phantom attempt must be captured with the updated panel log. [Details and steps](../JRSKILL_WALLET_PURCHASE_TEST.md).

PURCHASE-10 further separates quote/signature/expiry/message rejection codes after log (76) showed rejection eight seconds after quoting. If the signed message differs, public expected/actual metadata and instruction hashes are logged separately without transaction bytes or session credentials. **25 Python tests passed**, including a signed extra-instruction case that remains rejected. The real Phantom difference has not yet been identified; no new on-chain write or hardware test occurred.

## Phantom ComputeBudget compatibility — PURCHASE-11

Log (77) located the difference: the signed message prepended ComputeBudget price/limit instructions, while the purchase data/accounts, buyer and blockhash were unchanged. Signature/quote were valid; the server rejected before relay. Parameters imply 0.000075 test SOL priority fee. This explains the rejected attempt; it does not prove a purchase.

PURCHASE-11 accepts only the bounded price/limit pair before the identical purchase, preserving all original signer/writable privileges. Quotes explicitly show a 0.0001 test SOL priority allowance, network fee ceiling and maximum total. The signed message's RPC fee and balance are checked before relay. Unknown/extra instructions, privilege changes, purchase mutations, excessive fees and duplicates remain blocked. **29 Python and 21 JS tests passed**, including the observed parameters with simulated RPC/signatures; the bundle was rebuilt. The real Phantom purchase test remains pending. Program, firmware and JSON are unchanged; no new deploy or on-chain transaction was performed. [Policy and test steps](../JRSKILL_WALLET_PURCHASE_TEST.md).

## Real Phantom purchase and physical execution — confirmed

The user's log (78) records an absent license at 11:44:34, a quote, Phantom signing, relay at 11:44:47 and owned=true at 11:45:18. [Purchase transaction](https://explorer.solana.com/tx/3SSmCuBpQY2qh2EiwY94D3vn1nntxs97pC5kF4qSXpvKmcpKzsVyGfY1bTPN4qBrCJGprzH5CS6Nq8V1wXNRfFP9?cluster=devnet) was independently read through Devnet RPC: **finalized slot 507765827, no transaction error**. Buyer `6rinyiBQyS4RxLeuX62yJqxgwiJqWXiyAbxXSS5LX85R` now owns license `7tPf4YSd7P6PzBkmseW5FrwnG8P238gZTVG8Rj2v9v45` for the shared checkpoint Skill, model 00.

Exact results: 0.5 test SOL to the creator, 0.5 to the treasury, 0.0013462 license deposit, **0.00008 effective network fee** (0.000075 priority + 0.000005 base), total buyer debit **1.0014262 test SOL**. Post-transaction balance: 3.9985738. Fee stayed below the accepted 0.000105 ceiling. [Public RPC proof](assets/day6/license-purchase-phantom-proof.json).

The log records successful local execution at 11:45:43–46, then Devnet fetch with 128 bytes/hash verified at slot 507766081 and source=solana-devnet execution at 11:45:49–52, result=ok. The user visually confirmed the robot's face changes: happy -> surprised -> thinking -> happy -> neutral. [Selected Skill log evidence](assets/day6/panel-log-78-skill-excerpt.txt).

**Real Phantom purchase and physical execution are confirmed.** This does not prove authorization enforcement: the experimental executor still runs independently of license ownership. Next: gate execution by authenticated wallet/license and test licensed/unlicensed wallets, account changes and unavailable RPC. No new program/firmware deployment or code modification occurred in this evidence checkpoint.

## License gate implemented — physical test pending

Panel **JRBOT-PANEL-V1S-LICENSE-12** now requires authenticated wallet ownership before executing the Devnet Skill. The backend issues a session-bound, expiring permit for the fixed sequence and checks finalized license ownership again before each Serial command. Commands must match the sequence; account/session changes, absent license, unavailable RPC, expired/replayed permits and unexpected commands block further dispatch. The browser also drops stale results and stops subsequent commands on wallet changes.

**34 Python and 26 JS tests passed** (21 wallet + 5 executor), with RPC/Serial simulated for the new gate; the wallet bundle was rebuilt. No firmware, program, JSON or Recipe changes/deploy were needed. Real hardware licensed/unlicensed execution tests for this new gate remain pending. The earlier successful run in log (78) preceded the gate and is not a negative authorization proof. Local execution/manual controls remain development tools without licensing; this gate controls the panel's Devnet flow, not firmware DRM. [Architecture, limits and physical test plan](../JRSKILL_LICENSE_EXECUTION_TEST.md).

## License gate — real user test passed, log (79)

New wallet `8zQwpe3qVBzoPMasLQSbeagtmKzqL1JLG4iEApznUySo` connected/authenticated, then returned owned=false. Three Devnet execution attempts at 12:56:01, 12:56:12 and 12:56:33 were blocked with execution_license_absent. No authorized command dispatch for this wallet appears in the log. This is a missing-license rejection, not an insufficient-balance rejection.

The user then disconnected and returned to licensed wallet `6rinyiBQyS4RxLeuX62yJqxgwiJqWXiyAbxXSS5LX85R`. Authorization succeeded at 12:56:59, Skill data was verified at slot 507783945, six commands were dispatched and execution finished with result=ok/source=solana-devnet at 12:57:10. The user reported the robot test succeeded. Independent finalized RPC reads confirmed the new wallet remains unlicensed and the original wallet licensed. [License state proof](assets/day6/license-gate-wallets-proof.json), [selected log (79)](assets/day6/panel-log-79-license-excerpt.txt).

The positive/negative panel-flow proof is now recorded. Wallet switching happened between attempts; mid-run switching/disconnection and RPC failure are not claimed as physically validated by this log. Local development controls remain independent. This checkpoint changes documentation/evidence only, with no new build/deploy.

## Interruption and recovery — user checkpoint accepted, log (80)

The user performed further robot tests and considers the results very satisfactory. The licensed Devnet flow completed at 13:04:56. During later active sequences, wallet disconnection at 13:05:01 and 13:05:41 blocked subsequent execution; no successful completion was recorded for those attempts. Reauthentication restored successful Devnet execution at 13:08:10. A connection event for the same wallet at 13:08:19 also invalidated an active sequence.

The unlicensed wallet was rejected again at 13:09:17 with execution_license_absent. Returning to the licensed wallet produced result=ok at 13:09:52. At 13:10:05, an I/O failure (Remote end closed connection without response) blocked an active sequence. After reauthentication at 13:10:25, a new authorized sequence completed at 13:10:37. [Selected log (80) evidence](assets/day6/panel-log-80-interruption-excerpt.txt), [technical results and limits](../JRSKILL_LICENSE_EXECUTION_TEST.md#resultado-complementar--interrupcao-e-recuperacao-log-80).

Local-file execution completed at 13:05:21 despite wallet disconnection: this is the intended independent development mode. These observations validate the recorded panel-flow interruptions and recovery; the log does not identify the physical cause of the I/O failure or isolate a switch to a different wallet during an active sequence. Already dispatched commands cannot be undone, and the gate is not firmware DRM.

**The user accepts this checkpoint; no additional simulations or repetitions are requested.** This update records existing user tests only. No code change, new automated test, build, program/firmware deployment or payment occurred.

## Development panel reorganization — LAYOUT-13

The user clarified product boundaries: the web marketplace will handle discovery/purchases, and the robot app will handle Skill selection/execution. The current panel remains a development tool. **JRBOT-PANEL-V1S-LAYOUT-13** organizes existing controls into Robot, Skills and Diagnostics. Skills shows wallet, known test license and execution; the Devnet purchase and independent local test are collapsed. Purchase quotes show the price, revenue split, license deposit, network fee ceiling and maximum authorized total, with full addresses/details available separately.

Existing authentication, explicit purchase acceptance, signed transaction validation and per-command license authorization remain unchanged. No general Skill discovery/selection is implemented yet. Firmware, contract, frozen JSON and Recipe are unchanged, with no deployment.

**34 Python and 26 JS tests passed**; the wallet bundle was rebuilt. Local browser inspection confirmed navigation, collapsed controls and diagnostic log access without a connected wallet/robot. Real Phantom/hardware testing of this layout remains for the user. [Layout and test instructions](../JRSKILL_PANEL_LAYOUT_TEST.md). Next: license discovery/Skill selection, then the robot app.
