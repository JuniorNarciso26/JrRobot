# JrSkill License PDA 00 — Implementation and Local Proof

**Date:** October 2, 2026. **Development branch:** `V1s-00` only. Commercial tracking: [#46](https://github.com/JuniorNarciso26/JrRobot/issues/46); technical tracking: [#33](https://github.com/JuniorNarciso26/JrRobot/issues/33).

## Results and evidence limits

| Verification | Recorded result |
| --- | --- |
| Rust implementation and market/Offer/purchase/read scripts | Completed |
| Anchor 1.1.2 SBF build and IDL generation | Passed in WSL |
| `npm test` | 6 tests passed |
| `npm run test:idl` | 2 tests passed; original Skill codec preserved |
| `anchor test --validator legacy ...` | 2 integration tests passed, including the commercial proof below |
| Public read of original Devnet Skill | Passed at finalized slot **506702089** |
| Devnet commercial-program upgrade | Performed by user; RPC confirmed deployment slot **506708700** |
| First Devnet license purchase | October 5, 2026; finalized at slot **507724452** |
| Phantom panel purchase / physical license enforcement | **Not implemented in this intermediate checkpoint** |

Environment: Ubuntu/WSL, Node **24.10.0**, Anchor **1.1.2**, Solana CLI **3.1.10**. The local validator uses `[test] upgradeable = true` because market initialization verifies the program's upgrade authority. Without it, the locally loaded program lacks the required administrative authority. [Official Anchor configuration](https://www.anchor-lang.com/docs/references/anchor-toml#upgradeable).

**Program ID:** `Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454`.

Local SBF artifact SHA-256: `e7499f6e162354a741ce548f7d26edcd57bda22a2113efded63945315825b5aa`. This identifies the locally built artifact; it is **not** proof of a reproducible build or the actual Devnet upgrade by itself.

## Verified local cases

Tests used separate disposable wallets for administrator, creator, treasury, buyers A/B, and invalid attempts. Local test price: **100,000,001 lamports**, commission **5,000 bps**.

Buyer A received the correct License PDA referencing buyer, Skill, Offer, and model. Creator received **50,000,001 lamports**; treasury received **50,000,000**. Integer rounding preserved the exact total. Buyer A paid the price plus the rent deposit for a **137-byte** account; the administrator paid the test transaction fee.

Buyer B initially had no License. After purchasing the same Skill, B received a distinct License PDA. A duplicate purchase attempt by A did not alter recipient balances.

The following were rejected: initialization by an unauthorized third party, commission above 10,000 bps, Offer creation by another signer, zero price, redefining an existing market configuration or Offer, insufficient accepted price maximum, wrong creator or treasury, and a buyer equal to a recipient.

Two rollback tests sent failing transactions with preflight disabled and checked recorded failure and post-transaction balances/accounts:

- Enough funds to create the account and pay the creator, but not to pay the commission: **both** transfers rolled back and no License was created.
- A successful JrSkill purchase instruction followed by a **failing second instruction**: the entire transaction rolled back the transfers and License even though the first instruction itself succeeded.

Network fees remain separate and can be charged to the fee payer even for failed on-chain transactions.

Client integer tests covered commission flooring, 0/10,000 bps, maximum `u64`, and out-of-range input. Rust uses `u128` intermediates. No real purchase of a maximum-`u64` amount was claimed.

The original Skill create/read test still passed. The commercial test also preserved the original bytes after purchases and failures. Independent Devnet read:

- PDA `8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux`.
- **128-byte** payload.
- Hash `416d6af34eada998a5f46595e0355a5bdfd7dba86faefe312dbc2afcd26d907f`.
- Git blob `ecb748e1bbce58f381e6be0cda68725b2f7ae73d` preserved.

## Reproducing the local tests in VS Code / WSL

From `/home/user/JrRobot`, confirm `git branch --show-current` returns `V1s-00`. With a clean checkout, update **only** that branch:

```bash
git pull --ff-only origin V1s-00
cd solana/jrskill-solana
npm ci
npm test
```

Preserve the existing program key; do **not** generate another or run `anchor init`. Confirm its public address before building:

```bash
solana-keygen pubkey target/deploy/jrskill-keypair.json
# Expected: Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454
anchor build --provider.cluster devnet
npm run test:idl
if [ ! -f target/local-test-wallet.json ]; then
  solana-keygen new --silent --no-bip39-passphrase --outfile target/local-test-wallet.json
fi
anchor test --validator legacy --provider.cluster localnet --provider.wallet target/local-test-wallet.json
```

These tests require a **clean validator**, started by Anchor. If port 8899 is already occupied, stop only the known local validator before retrying.

## Recorded Devnet upgrade and subsequent purchase

The user compiled, passed six client tests, two IDL tests, and two local integration tests, then upgraded the **same program**.

Reported upgrade signature: `3Bdw92brd5inoDoMmg7q3SzcMrpDsctoL5pd63r71kMbSM37k6yHCvo6nvpQrF7HBxCXoUoZaF6mmRDPSDWnNYqT`.

The CLI increased program data size from **191,976 to 331,392 bytes** and updated IDL. Independent RPC confirmed last deployment at slot **506708700**, unchanged upgrade authority, and unchanged ProgramData. A subsequent public verifier at finalized slot **506709949** confirmed the original **128 bytes and hash**.

The commands below record the **upgrade already performed**, not an instruction to repeat it without review. Market configuration, Offer creation, and the first purchase are documented later. Use Devnet/test SOL; do not replace keys or remove upgrade authority.

```bash
export ANCHOR_PROVIDER_URL=https://api.devnet.solana.com
export ANCHOR_WALLET="$HOME/.config/solana/id.json"
solana address --keypair "$ANCHOR_WALLET"
# Original authority: 3Sce1sfA6q2m2mNr2VhyGfoTePa3vA9WjYYq2JYA5mij
solana program show Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454 --url devnet
npm run verify:public
anchor build --provider.cluster devnet
npm run test:idl
anchor program deploy --provider.cluster devnet --provider.wallet "$ANCHOR_WALLET"
npm run verify:public
```

Stop before deployment if program keys/authority do not match or tests fail. An upgrade updates the same program and IDL while preserving the existing Skill account. If more program-data space is required, record the exact error before continuing; **do not deploy to a different Program ID**.

Following the upgrade, the proof specifies an explicit public treasury wallet and test commission. To observe separate shares, use a treasury wallet different from creator and buyer. Initialize once using the authorized administrator. The Offer requires the original Skill creator wallet:

```bash
# Replace PUBLIC_TREASURY_ADDRESS with the intended public test wallet.
npm run market:devnet -- configure PUBLIC_TREASURY_ADDRESS 5000
npm run market:devnet -- offer 8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux 1000000000
```

The **5,000 bps** and **1 test SOL** are explicit examples, not approved commercial price lists. Model 00 does not support resetting market configuration or the Offer, so verify arguments before submission.

For an isolated CLI purchase, use a **disposable local test wallet** different from payment recipients and funded on Devnet. `ANCHOR_WALLET` points to a locally controlled key; do not export the Phantom seed or private key. `--quote` reads costs without purchasing, while `--send` explicitly submits a transaction:

```bash
# ANCHOR_WALLET points to the disposable LOCAL BUYER wallet.
npm run market:devnet -- buy 8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux 1000000000 --quote
npm run market:devnet -- buy 8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux 1000000000 --send
npm run market:devnet -- read PUBLIC_BUYER_ADDRESS 8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux
```

The quote itemizes price, shares, rent deposit, and estimated network fee. If a valid License already exists, the client only reads it and submits no new transaction. Public reads require locally generated IDL, npm dependencies, and Devnet RPC but not the buyer's private wallet. Validation covers owner, discriminator, derived PDA, buyer, Skill, Offer, model, and recorded shares.

At this intermediate checkpoint, a Phantom wallet holding five test SOL was ready for a future panel purchase, but this CLI step did **not** submit from that wallet.

## First Devnet purchase — October 5, 2026

**Market configuration:** `6k8BdUgobWSuwjUTiTvGgBBtK8Zc2vpfpV7vJC8WEe9D`; treasury `Ex9pYvpA96dkZF5efdooEU2pQvWr5owpAZPiqXtLYxnh`; commission 5,000 bps.

Reported setup signature: `2YcAUZwRzFBhFMsACpLueaRBRpRTEswrLNqWqSsaXGSgkEeNiUDeELSg9dger2ewvRn9VGnXbmX5Ky57b4MtZFRt`. A repeat configure attempt failed in simulation because the account existed; its settings were retained.

**Offer:** `A2n5s6TYEH9oFZNFE1owxN5nEh18WGDHnEvrc5QvBgLR`; original Skill, price **1,000,000,000 lamports**. Reported signature `oASMhPRZB8aA1biQDRsVJeC5dyqA3dYH2QQxCSQ852RFmkw5ogBq7JtcUz9qJKZBtGvDjDJ9vpBBw3E67sZNsLd`. Pre-purchase RPC queries confirmed JrSkill ownership, price, and commission.

**CLI buyer:** `E5F1ztxZwgogcwWaryB6uCLXp3iZ2YA6MBzjwv6R51dE`. `--quote` did not send a transaction. `--send` confirmed signature `4dwpsPA6GrZNhLuJhmdWQPJSv1r3QLnpMnxGrY4PvdQj2AAEzs2aeWu9YrBS7Kut5rhDgMenq9U7wiEd1GnMLKiY`.

**License PDA:** `5wjuV3W2fJo9o8U5oqfjcnbUULGxKqRxLXK9AgUvqADR`, `verified=true`.

An independent read-only RPC verified Devnet genesis, finalized transaction at slot **507724452**, `meta.err=null`, and correct License owner, discriminator, PDA, buyer, Skill, Offer, model 0, and shares.

[Archived RPC evidence](../06-colosseum-history/HACKATHON_DEVLOG/assets/day6/license-purchase-proof.json).

| Transfer or change | Lamports | Test SOL |
| --- | ---: | ---: |
| Creator received | 500,000,000 | 0.5 |
| Treasury received | 500,000,000 | 0.5 |
| License PDA rent deposit | 1,346,200 | 0.0013462 |
| Actual network fee | 5,000 | 0.000005 |
| Total buyer debit | 1,001,351,200 | 1.0013512 |
| Buyer balance after transaction | 3,998,648,800 | 3.9986488 |

Market configuration, Offer, and Skill balances were unchanged; neither creator nor treasury shares were reduced by costs. These are **historical pre/post transaction balances**, not a promise of future balances. All funds were **test SOL**, not commercial revenue.

A public Skill verifier after purchase confirmed finalized slot **507725504**, **128 bytes**, original hash, and `matches_checkpoint=true`.

### Repeat buyer A and independent buyer B

Buyer A repeated `buy --send`. The client returned `already_exists: true`, the same buyer and License PDA, and **no new signature**. This proves client-side existing-License behavior, not rejection of a second submitted on-chain transaction.

Buyer B `Dyf2qbLSn7pW7khP4z4xWykDr7Kuw5uMqmxzMFRuU6vs` initially returned `exists: false`, with derived PDA `CdvpW2VoTK4E1H39d688g9mW9vFtLwasqXrdiVuModLo`. After a quote and purchase of the same Skill for 1 test SOL, lookup returned `exists: true`, model 0, and 0.5/0.5 shares.

Buyer B transaction signature: `oZkPZsQXtoYfC5FhjHjKS9zoyhtGsVkERdZaa6HKuBB4Y2x4Fuqif2TEa1ZErUmEsN8HZb5N3gmjk15nLNNd9jD`.

Independent RPC confirmed finalized slot **507731090**, `meta.err=null`, and a second valid License PDA for the same Skill, distinct from buyer A's.

Buyer B's debit was **1,001,351,200 lamports**; creator +500,000,000, treasury +500,000,000, License deposit 1,346,200, network fee 5,000.

[Buyer B balances and RPC proof](../06-colosseum-history/HACKATHON_DEVLOG/assets/day6/license-purchase-b-proof.json).

**Positive A/B Devnet purchasing proof completed.** Duplicate/rollback negative contract cases were verified on the local validator; this document does **not** claim new Devnet negative tests. Phantom purchase, license discovery, and physical execution enforcement were still pending at this checkpoint.

## Release identification — Version correction

The commercial upgrade above had been built/deployed with older metadata **0.1.0**. A subsequent repository correction changed npm scripts and Rust crate metadata to **0.2.0**, aligned lockfiles, and set the panel identifier to **`JRBOT-PANEL-V1S-WALLET-07`**.

The panel version was subsequently read only from `app.APP_VERSION`, without an override from the stable wrapper.

That metadata correction did not change instructions, account layouts, JSON, firmware, or implement panel purchasing. Locally generated IDL used version **0.2.0**, whereas IDL metadata deployed during the previous upgrade remained **0.1.0**. This record does not claim a further metadata-only Devnet deployment.

The commercial program already deployed remained compatible with the same instructions/scripts. Changing the displayed panel version did not require redeployment.

After `git pull`, close and reopen `PAINEL.bat` for WALLET-07. In WSL, `npm ci`/`npm test` show 0.2.0; `anchor build` produces local corresponding IDL. Refresh the panel with Ctrl+F5.

## Stage 4D — Browser purchase

Panel `JRBOT-PANEL-V1S-PURCHASE-08` implemented quotation, Wallet Standard Devnet signing, verified submission, and finalized License lookup for the known reference Skill.

Software tests passed. At the time of this checkpoint, real Phantom purchase still required user testing; no new Devnet purchase/deployment was performed in that particular release. The experimental executor was still independent of License enforcement. See [architecture, limits, and procedure](JRSKILL_WALLET_PURCHASE_TEST.md).

### Later real result — PURCHASE-11 / Log 78

A real Phantom purchase was confirmed at finalized slot **507765827**, `meta.err=null`.

Buyer wallet `6rinyiBQyS4RxLeuX62yJqxgwiJqWXiyAbxXSS5LX85R` received License `7tPf4YSd7P6PzBkmseW5FrwnG8P238gZTVG8Rj2v9v45`.

The **1 test SOL** price was split **0.5/0.5**; rent deposit 0.0013462, actual network fee 0.00008, total debit 1.0014262 test SOL. Independent RPC verified the License and payment shares.

The user also confirmed physical local and Devnet execution with `result=ok` in the log. License-enforcement tests were tracked separately. [Evidence and limitations](JRSKILL_WALLET_PURCHASE_TEST.md#resultado-real--phantom-e-execucao-fisica-log-78).
