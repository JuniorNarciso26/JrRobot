# JrSkill — License Purchase Proof, Commercial Model 00

## Scope and status — October 2, 2026

This was the next isolated proof on `V1s-00`, tracked in [Issue #46](https://github.com/JuniorNarciso26/JrRobot/issues/46). [Issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33) covered integration, panel authentication, and physical execution.

**Recorded status:** implementation, SBF/IDL compilation, local tests, and Devnet commercial-program upgrade completed on **October 2, 2026**. The first CLI License purchase was confirmed on Devnet on **October 5, 2026**. A finalized RPC read verified **0.5/0.5 test SOL** distributions and the buyer's License PDA at slot **507724452**.

At this intermediate checkpoint, Phantom/panel purchases and other Devnet tests remained pending. Identifiers: scripts/crate **0.2.0**, panel `WALLET-07`; the previous upgrade's metadata still showed **0.1.0**. See [purchase checkpoint 00](JRSKILL_LICENSE_MODEL_00_TEST.md) for evidence and limitations.

Commercial model 00 covers **purchase, revenue distribution, and license recording**. On October 2, the project owner chose to run the panel's wallet-connection proof first, as [Stage 4A](JRSKILL_WALLET_CONNECTION_TEST.md), without implementing purchasing or licensing in that step.

Commercial numbering 00/01/02 is independent of JrSkill JSON v1 and firmware `JrBot_V1S_00`.

## Minimum Model 00 design

Keep a single JrSkill program and preserve the existing Skill layout. No new instruction may change the original payload, authority, schema, or hash.

| Entity | Responsibility |
| --- | --- |
| Existing Skill | Content and creator identity |
| Market configuration 00 | JrBot commission wallet and test percentage |
| Offer 00 | Skill, creator, price in lamports, and allocation terms |
| License PDA | Buyer wallet, Skill, model 00, and purchase record |

Proposed license derivation: `["license", buyer, skill_pda]`. Buyers A and B can license the same Skill without duplicating JSON. This proof permits only **one license per buyer and Skill**.

The market configuration must be initialized by an authority verified **by the program**; the first caller must not be allowed to choose a commission wallet arbitrarily. Offer creation requires the actual `Skill.authority` signature. A recipient address supplied by the client cannot substitute for that authority verification.

For model 00, market configuration and Offer terms were proposed as immutable during the proof. Changes to price/commission, delisting, and migration were deferred. Commercial configuration must not change who may publish an ordinary Skill.

## Proposed purchase flow

```text
Buyer reads Offer and accepts its terms
  → signs purchase with an accepted maximum price
  → program validates Skill, Offer, recipients, and signatures
  → distributes payment to creator and JrBot
  → creates buyer's License PDA
  → client confirms transaction and queries license
```

Payment and issuance must happen in **the same transaction**. The License is associated with a specific Skill and the purchased content. A failure must not leave partial payment or a License without the required payment. Network transaction fees are separate and may still apply when a transaction fails.

The proposed buyer pays the Skill price, network fee, and rent deposit for the License account. Those amounts must be displayed separately before signing; the model's acceptance criteria were tracked in Issue #46.

All arithmetic uses **integer lamports**, with overflow checks and commission constrained to **0–10,000 basis points**. Proposed rounding:

```text
commission = floor(price * basis_points / 10000)
creator    = price - commission
```

The two shares therefore always total the purchase price.

A **1 test SOL** price and **5,000 basis points (50%)** commission were test examples, not commercial pricing. Scripts must accept terms explicitly and display the split before the purchase. Opening the panel or reading an Offer must **never** initiate an automatic purchase.

A client may detect an existing License before sending a transaction. The on-chain program must also block duplicate purchases, including simultaneous attempts. A duplicate transaction may incur a network fee, but it must not pay the License price again.

## Two-stage proof

### 1. Local validator

Use distinct disposable wallets for administrator, creator, JrBot commission receiver, buyer A, and buyer B. Validate:

- The authorized creator can create an Offer; another signer cannot sell that Skill.
- An arbitrary caller cannot seize the market configuration.
- Buyer A pays correct shares and receives a License with correct buyer/Skill/model.
- Buyer B initially has no License; after purchasing, B has a **different PDA for the same Skill**.
- A duplicate purchase does not repeat payment.
- Excessive price versus accepted maximum, wrong recipient, insufficient funds, or invalid Offer/Skill cannot cause partial payment or issuance.
- A failure after an intermediate payment operation rolls back the entire purchase.
- Invalid commission/overflow is rejected; rounding is correct.
- The previously published Skill remains readable with identical bytes and hash.

To compare recipient balances precisely, use recipients who are **not** paying the transaction fee. Separate network fees and the License PDA rent deposit from the buyer's balance changes. Record balances and absence of accounts after rollback tests.

### 2. Solana Devnet

Only after successful compilation and local testing, preserve the original program key, confirm compatibility, and prepare an upgrade of the **same Program ID**. The initial design document itself did not deploy that upgrade.

Every writing client must verify the Devnet genesis hash before changing on-chain state. Mainnet is excluded. Use disposable local test wallets; never commit private keys.

Record a signed purchase with public addresses, exact price, allocations, deposit, fee, and License PDA. Public verification must check owner, account type, buyer, Skill, and model; merely finding any account at an address is insufficient.

## Deliverables for the next implementation

- Minimal commercial accounts and instructions in the existing program.
- Separate scripts for market setup, Offer creation, purchase, and License lookup.
- Local tests for purchase, distribution, access controls, duplicates, and rollback.
- Evidence distinguishing static review, compilation, local testing, and Devnet.

This first proof excluded panel-wallet purchasing, firmware changes, new Recipes, JSON v1 changes, NFT, resale, and marketplace expansion. Wallet connection/authentication existed separately. A future License check does not prevent copying a publicly readable Skill JSON.

## Decisions implemented on October 2, 2026

- Immutable market configuration `["market", "00"]`, initialized only by a signer matching the JrSkill program's **ProgramData upgrade authority**.
- Immutable Offer `["offer", "00", Skill]`, signed by `Skill.authority`.
- License `["license", buyer, Skill]`, model 0, with no modify, transfer, or close instructions in this version.

A purchase must match Offer/market recipients and satisfy the maximum accepted price. Model 00 requires the buyer to differ from creator and commission receiver, so self-transfers are not misrepresented as external payments. Creator and treasury may coincide; in that case they receive the combined price. Recipient wallets must be System Program accounts.

The buyer pays the price and License deposit; the transaction fee payer pays the network fee. In the purchase script, both roles are the buyer. In local tests, the administrator pays the fee so buyer/recipient balance changes can be compared exactly.

Price and commission are explicit arguments, not a fixed commercial price list. Commission math uses an intermediate `u128` and `u64` result, including the maximum `u64` price without multiplication overflow.

New instructions: `initialize_market`, `create_offer`, and `buy_license`. Existing `Skill` discriminators, seeds, fields, and `create_skill` were preserved. Previous errors remain in the same order.

The program remains **upgradeable**; immutable account behavior describes the current instruction set, **not** a promise that upgrades will always be impossible.

## Evolution

Close the results of model 00 in Issue #46 before defining model 01. Record any new licensing rule's effect on previous buyers. Wallet connection/authentication, License discovery, and pre-execution authorization are tracked in [Stage 4](JRSKILL_SOLANA_STAGE4.md) after the isolated purchase proof.
