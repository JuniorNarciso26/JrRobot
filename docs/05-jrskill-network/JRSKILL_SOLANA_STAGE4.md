# Stage 4 — Licensing Through License PDAs

## Architecture decision — October 1, 2026

The project owner chose **License PDAs** instead of NFTs to support custom license rules and the future JrSkill marketplace. This decision was recorded on `V1s-00` and in [Issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33).

**Updated status (October 5, 2026):** CLI purchases by buyers A/B and a real Phantom purchase were finalized on Devnet, with verified License PDAs and payment distributions. Log 78 records panel purchase and successful local/Devnet execution (`result=ok`); the user visually confirmed OLED expressions. Logs 79 and 80 document licensed execution, unlicensed blocking, interruption after disconnection or communication failure, and recovery.

The accepted development state was `JRBOT-PANEL-V1S-SKILLS-14` and scripts/crate 0.2.0. [Skill discovery](JRSKILL_SKILL_DISCOVERY_TEST.md) was implemented with the earlier layout retained; LICENSE-12 recorded physical authorization tests.

Related milestones: [Stage 4A wallet connection](JRSKILL_WALLET_CONNECTION_TEST.md), [4B authentication](JRSKILL_WALLET_AUTH_TEST.md), [4C Devnet/balance](JRSKILL_WALLET_DEVNET_TEST.md), [4D browser purchase](JRSKILL_WALLET_PURCHASE_TEST.md), and [commercial model 00](JRSKILL_LICENSE_MODEL_00_TEST.md). Discovery listed Skills licensed under wallet model 00, while generic Skill selection/execution was still planned at this checkpoint. [Stage 4E](JRSKILL_LICENSE_EXECUTION_TEST.md) covers authorization and physical tests.

The license gate protects the panel's **Devnet execution flow**; local development controls remain independent.

## Proposed model

- **Skill PDA:** executable content, schema, hash, and publisher.
- **License PDA:** authorization record tied to a wallet and one Skill.
- **JrSkill program:** instructions to issue and validate licenses.
- **Panel:** authenticates the wallet, discovers its licenses, and displays authorized Skills.

Proposed derivation, subject to detailed validation:

```text
License PDA = ["license", wallet, skill_pda]
```

Buyers A and B may own separate licenses for the same Skill without duplicating the payload. A License PDA is a **program data account**, not an NFT that necessarily appears automatically in a wallet UI. The JrSkill panel displays licensed Skills.

A PDA is not an independent executable smart contract. Skill and License accounts can reside under the same program; a future marketplace contract could call its instructions through CPI.

## Licensing questions requiring separate decisions

- Who may issue licenses, and how the program verifies authority.
- Perpetual or expiring licenses, including revocation.
- Wallet-to-wallet transfers and access recovery.
- License rights for a specific Skill version versus future upgrades.
- Atomic test purchase and issuance, with commercial model 00 tracked in Issue #46.
- Wallet proof of control through signatures and correct discovery/validation in the panel.

Choosing PDAs does not automatically implement payments, resale, expiration, or recovery. These require separate specification, implementation, and tests. Wallet authentication cannot rely solely on a user-entered public address.

## Next proof as originally planned

On October 2, the user moved the wallet connection experiment earlier in the sequence. [Stage 4A](JRSKILL_WALLET_CONNECTION_TEST.md) tested connection only, without signature, purchase, or execution authorization. Authentication and isolated purchase/revenue-sharing License PDA tests followed in [Issue #46](https://github.com/JuniorNarciso26/JrRobot/issues/46), using the [model 00 design](JRSKILL_LICENSE_MODEL_00.md). The prices and revenue split in that design were examples, not final commercial terms.

Planned integration after purchasing:

```text
Connect and authenticate wallet
  → list licenses issued by JrSkill
  → verify permission for selected Skill
  → retrieve and validate payload
  → existing executor → Runtime API → JrBot
```

Compare licensed and unlicensed wallets. The unlicensed wallet must be rejected **before** any action reaches the robot. Public payloads remain copyable: license enforcement only applies to clients that honor these checks.

This design decision did not itself change the contract, JSON v1, Recipes, or firmware. Preserve existing Skill layouts and confirm compatibility before upgrades.

## References

- [Validated Stage 3](JRSKILL_SOLANA_STAGE3.md)
- [Day 4 Development Log](../06-colosseum-history/HACKATHON_DEVLOG/day4.md)
