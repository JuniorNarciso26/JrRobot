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

This proves one positive Devnet purchase. Duplicate/rollback cases have local-validator evidence; their Devnet follow-ups and a second buyer remain pending. Phantom purchase, licensed Skill listing and authorization before physical execution also remain pending. No firmware, JSON/Recipe or program code changed in this documentation checkpoint.

Detailed terms/results: [model 00 checkpoint](../JRSKILL_LICENSE_MODEL_00_TEST.md). History: [commercial issue #46](https://github.com/JuniorNarciso26/JrRobot/issues/46), [technical issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33).
