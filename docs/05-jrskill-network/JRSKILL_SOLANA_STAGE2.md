# JrSkill Network — Stage 2: Skill PDA on Solana Devnet

**Date:** September 30, 2026. **Development line:** `V1s-00`. [Technical tracking: Issue #33](https://github.com/JuniorNarciso26/JrRobot/issues/33).

**Final status: Stage 2 completed.** The original JSON v1 was published to Solana Devnet and recovered with **byte-for-byte equality**. Intermediate checkpoints below are preserved as the historical sequence; the final evidence is in the completion section.

## Objective and baseline

Publish `tools/jrbot_frontend/jrskill/skills/minimal_recipe_01.json` and retrieve the **exact original bytes**, without changing the JSON v1 validated in Stage 1.

Before implementation, remote `V1s-00` was at `6fee26ec0e1168207ce245c41731db0ca7ac9859`. Issue #33 was open; local execution had been validated, and Skill PDA publication was the next task. Earlier references to `test/jrbot-v1s-00` in that issue are historical.

The original Git blob (LF endings) had **128 bytes**, SHA-256 `416d6af34eada998a5f46595e0355a5bdfd7dba86faefe312dbc2afcd26d907f`. The Windows checkout used in some local tests (CRLF endings) had **137 bytes**, SHA-256 `b4b75ec8bf66012e7d3308d046d230998eb99cdc2b2386d4299791f5ed3f0714`.

The versioned Git blob was not changed. Publication scripts hash bytes read from disk **including spaces and line endings**, without `JSON.stringify`, minifying, or expanding a Recipe. Different checkout line endings create a different hash/PDA. Proofs must compare against the exact file used for publication; record the printed publication hash with the logs.

## Architecture

```text
Existing JSON (UTF-8 bytes)
    ↓ client validates JrSkill v1 and calculates SHA-256
create_skill(schema_version, payload_hash, payload)
    ↓ authority signs and pays rent/fees
Skill PDA: ["skill", authority, payload_hash] + Program ID
    ↓ RPC + owner + discriminator + Borsh deserialization
client verifies authority, schema, hash, byte-for-byte equality
    ↓
artifacts/<PDA>.json (not executed on the robot)
```

The minimal single-file Anchor program was placed in `solana/jrskill-solana`, pinned to **Anchor 1.1.2** to match the WSL toolchain. The JavaScript client uses `@anchor-lang/core` **1.1.2** and `@solana/web3.js` **1.98.4** (Web3 API v1 compatible with Anchor). `Cargo.lock` and `package-lock.json` pin dependency resolution. Rust SHA-256 uses `sha2` **0.10.9**.

### Skill account

| Field | Type | Responsibility |
| --- | --- | --- |
| `authority` | `Pubkey` | Publisher and creation-fee signer |
| `schema_version` | `u8` | Stored-envelope schema version: 1 |
| `payload_hash` | `[u8; 32]` | SHA-256 of original payload bytes |
| `payload` | `Vec<u8>` | Original JSON without reserialization |

Reserved account space: `8 + 32 + 1 + 32 + 4 + 512 = 589 bytes`, including the Anchor discriminator and Borsh vector length prefix.

The **512-byte payload limit** keeps this proof within a small instruction/transaction. The client also verifies the **1,232-byte transaction size limit**. There was no chunking protocol in this stage.

### Rules and limits

- `create_skill` requires the payer/authority signature and correctly derived PDA.
- The program accepts only `schema_version=1`, a nonempty payload of up to 512 bytes, and a matching SHA-256.
- `init` permits one creation per authority/hash combination. No update, delete, close, registry, or License operation was present yet.
- Identical content published by different authorities results in separate PDAs; different content also results in a different PDA.
- The canonical PDA bump can be calculated; it does not need to be stored.
- The program stores and checks **byte integrity**, without interpreting JSON or resolving Recipes.
- Validation of `{v,run}` and individual v1 operations remains client-side. Another client could publish bytes that are not valid JrSkill. **Every executor must revalidate** them.
- Immutable data describes the current program instructions. The program still has an **upgrade authority**; the proof does not freeze or relinquish it.
- `recipe("face_sequence")` remains a reference in JSON, but the local Recipe is neither published nor executed on-chain at this stage.
- Publisher/reader scripts verify the **Devnet genesis hash**; on-chain tests accept only localhost.
- Preserve the versioned validated Devnet Program ID `Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454` and its existing program key. Synchronize **devnet and localnet** using the same key before build/deploy.

## Test plan

Complete commands are in [the Solana project README](../../solana/jrskill-solana/README.md).

| Proof | Acceptance criterion | Evidence level |
| --- | --- | --- |
| `npm test` | Original file preserved; schema v1, equality, and rejection of altered data | Local client test |
| `cargo check -p jrskill` | Host checks Rust types/macros | Host compilation check, not SBF |
| `anchor idl build` + `npm run test:idl` | Real IDL encodes instruction/account; transaction fits; returned bytes match | IDL build and local codec test |
| `anchor build` | Generates `target/deploy/jrskill.so` and IDL | SBF compilation |
| `anchor test --validator legacy --provider.cluster localnet` | Create/read, reject duplicate, wrong hash, schema 2, empty/>512 payload, and wrong PDA; failures leave no account | Local validator; not proof of Devnet |
| Devnet deployment | Executable Program ID on the correct cluster | Deployment alone does not prove Skill publication |
| `publish:devnet` | Confirmed transaction, PDA, and fields identical to original | On-chain write and read |
| `read:devnet -- <authority>` | Independently check owner/discriminator/hash/version/authority/bytes | Stage 2 completion criterion |
| Repeat `publish:devnet` | Same PDA, read-only verification, no replacement | Client repeatability |

The client rejects modified authority, version, hash, or payload responses. Program tests require a clean local validator because they exercise first-time creation of the frozen payload.

## Implementation checkpoints

Program, publication/read scripts, and tests were implemented. Recorded validation:

- **Host Rust check:** `cargo check -p jrskill` passed.
- **IDL build:** `anchor idl build -p jrskill -o target/idl/jrskill.json` passed.
- **SBF build:** `anchor build` generated `.so` and IDL; a placeholder ID initially differed from the laboratory key, so the user workflow required `anchor keys sync`.
- **Local tests:** three payload tests and two IDL/codec tests passed.
- **Local validator:** one on-chain test passed, including account creation, reading, and all negative cases. The `.so` was loaded using `solana-test-validator --bpf-program` with a placeholder ID and disposable wallet. Node ran on Windows; the validator ran in WSL on RPC port **18899**. The integrated `anchor test` command had not yet been run because Node/npm were missing from WSL PATH.

### Subsequent user WSL checkpoint — September 30, 2026

After selecting `--validator legacy` and synchronizing program IDs for devnet and localnet, the integrated `anchor test` **passed: 1 test, 0 failures, in 500.476268 ms**. All three payload tests and two IDL tests also passed in WSL.

Local program public key: `Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454`. At that moment, there was not yet Devnet deployment proof. The build showed successful release and test profiles. The user's logs were submitted to Issue #33.

### Devnet deployment checkpoint — September 30, 2026

The user rebuilt for Devnet, passed both IDL tests, and deployed. Logs reported `Deploy success` and IDL metadata address `2nKon1p32sHB7DWLrpcxh5x6z5s864KYM6RZ675baeaa`.

An independent `solana program show --url devnet` query verified:

- **Program ID:** `Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454`.
- **Upgrade authority:** `3Sce1sfA6q2m2mNr2VhyGfoTePa3vA9WjYYq2JYA5mij`.
- **Deployment slot:** **505968344**.
- **Binary size:** **191,976 bytes**.

The supplied log did not display the deployment transaction signature. At this intermediate point, the program was deployed, but publication of the JSON to a Skill PDA and independent reading were still pending. No firmware compilation or physical JrBot test occurred in this stage.

No new runtime functions, Recipes, licenses, or panel integration were added. Stage 3 depended on a completed Devnet round trip.

### Devnet network-guard fix before publication — September 30, 2026

The first user `publish:devnet` attempt was blocked **before submitting a transaction** because `DEVNET_GENESIS` was truncated.

The official RPC and `solana genesis-hash --url devnet` returned the full value:

```text
EtWTRABZaYq6iMfeYKouRu166VU2xqa1wcaWoxPkrZBG
```

The client was corrected to require that exact hash and report unexpected values. Regression tests were added for the full official value and rejection of another network, missing value, or truncated hash. Five payload/network tests and one read-only RPC query passed. That fix affected only the client and required no new program deployment; it was **not** yet proof of publication.

During installation, `npm audit` reported transitive dependency advisories in the Anchor/Web3 stack (`toml`, `stream-json`, `uuid`). The lockfile was preserved without `audit fix --force`. Dependencies require reassessment before production use.

## Stage 2 completion — Devnet, September 30, 2026

User-supplied logs confirmed publication with `result: "ok"`, `already_exists: false`, followed by an independent `read:devnet` with `result: "ok"` and `byte_equal: true`.

| Evidence | Value |
| --- | --- |
| Program | `Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454` |
| Authority | `3Sce1sfA6q2m2mNr2VhyGfoTePa3vA9WjYYq2JYA5mij` |
| Skill PDA | `8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux` |
| Publication transaction signature | `4MJJ8awPvv4LKjHHemodRbSKAwMgcJaUZRyudNP81ZzVGUao8cHXLskGadF5VyjVwNh4iKQw8dLNn9frnMthogC6` |
| Payload | **128 bytes** |
| SHA-256 | `416d6af34eada998a5f46595e0355a5bdfd7dba86faefe312dbc2afcd26d907f` |
| Transaction size | **410 bytes** |
| Reserved account space | **589 bytes** |
| Reported rent | **3,642,360 lamports** |
| Independent read equality | **`byte_equal: true`** |

The WSL-recovered file was `solana/jrskill-solana/artifacts/8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux.json`, kept locally outside Git. Its hash matched the original LF Git blob.

In addition to the user's publication/read logs, an independent Devnet RPC read verified the same PDA, bytes, hash, and `byte_equal: true`. The client checked owner, discriminator, authority, schema, hash, and byte equality.

**Stage 2 completion criterion achieved: original JSON → Devnet Skill PDA → identical recovered JSON.**

At this point, the stage had **not** integrated the robot executor, achieved physical execution from blockchain data, published a Recipe, or implemented licensing. Repeated publication returning `already_exists: true` remained an optional test not evidenced in these checkpoint logs.

The next stage was **Solana Skill → existing executor → Runtime API → JrBot**.

## Official references

- [Anchor 1.1.2](https://www.anchor-lang.com/docs/updates/release-notes/1-1-2)
- [Anchor TypeScript client](https://www.anchor-lang.com/docs/clients/typescript)
- [Anchor account constraints](https://www.anchor-lang.com/docs/references/account-constraints)
- [Anchor account space](https://www.anchor-lang.com/docs/references/space)
- [Solana PDAs](https://solana.com/docs/core/pda)
- [Solana transactions](https://solana.com/docs/core/transactions)
- [JrSkill Execution API](JRSKILL_API.md)
