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
