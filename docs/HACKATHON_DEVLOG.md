# JrSkill Network — Hackathon Development Log

This log records the day-by-day development of **JrSkill Network** during the **Crypto World's Fair / Colosseum Hackathon 2026**.

The purpose is to keep a clear public timeline of what was planned, implemented, tested and validated during the hackathon.

JrBot itself predates the hackathon. This log focuses only on the JrSkill Network work developed during the competition.

---

## 2026-09-27 — Project kickoff / registration

The JrSkill Network project was registered for the Colosseum hackathon.

The initial concept was defined around a simple question:

> Can a physical robot receive portable Skills whose distribution and license state are represented on Solana, while the robot executes only safe, approved capabilities?

The first scope was intentionally narrow:

- use JrBot as the physical reference platform;
- keep the ESP32 isolated from arbitrary downloaded code;
- use declarative Skills instead of native firmware extensions;
- use Solana later for publication, retrieval and licensing;
- keep the first physical proof small and observable.

Technical tracking was centralized in **Issue #33**.

No Solana integration was implemented on this kickoff day.

---

## 2026-09-28 — Day 1: architecture and development plan

Day 1 focused on preparing the technical path before touching blockchain integration.

### Decisions

A dedicated experimental line was created:

```text
V1s-00
firmware: JrBot_V1S_00
```

The V1S line was derived from the validated JrBot V1.7 baseline so the hackathon experiment could evolve without mixing with the stable product line.

The main architecture was defined as:

```text
Skill
  ↓
Skill Executor
  ↓
approved capability
  ↓
Runtime API
  ↓
JrBot hardware
```

A key decision was made early: the Skill must never contain arbitrary C/C++ code, direct GPIO access or executable binaries.

Instead, a Skill describes behavior and the local executor decides whether each requested capability is allowed.

### Development sequence

The planned proof path was organized as:

```text
local external Skill
        ↓
local executor
        ↓
physical JrBot
        ↓
Solana Skill storage
        ↓
Solana → executor → JrBot
        ↓
Wallet / License PDA
```

This order was chosen to avoid debugging blockchain, execution logic and hardware at the same time.

Relevant experimental branch start:

- `7cdb689c7525fe41ec405824782fc31b1d7a57c3` — initial `JrBot_V1S_00` variant.

Day 1 ended with the architecture and test direction defined. No blockchain proof was attempted yet.

---

## 2026-09-29 — Day 2: local Skill execution and physical validation

Day 2 focused on proving the execution model before moving to Solana.

### Step 1 — Runtime API baseline

An external JSON test sequence was added to the PC development panel.

The sequence used only capabilities that already existed in Runtime API 1.1:

- `capabilities`;
- `get`;
- `face`.

The test path was:

```text
external JSON
    ↓
PC test panel
    ↓
Serial
    ↓
Runtime API 1.1
    ↓
face capability
    ↓
SSD1306 OLED
```

The physical robot executed:

```text
surprised → thinking → happy → neutral
```

The OLED transitions were visually confirmed and the test ended with:

```text
JR_SKILL_API_TEST result=ok
```

Relevant commits:

- `8625fb3f024a87719b1589c05d5847a9810009da` — Runtime API sequence through Serial;
- `904f922f5826703d780fc03a57ed760d49603414` — panel identification and test error handling.

### Step 2 — minimal JrSkill JSON v1

After the low-level path worked, the verbose test JSON was intentionally reduced to a minimal Skill format:

```json
{
  "v": 1,
  "run": [
    ["face", "happy"],
    ["wait", 500],
    ["recipe", "face_sequence"],
    ["face", "neutral"]
  ]
}
```

The current schema contains only:

- `v` — JrSkill schema version;
- `run` — ordered function calls.

Transport details, request IDs, firmware version, logs and expected test results are deliberately kept outside the executable Skill payload.

### Step 3 — reusable Recipes

A local Recipe library was introduced.

The first Recipe was:

```json
{
  "v": 1,
  "run": [
    ["face", "surprised"],
    ["wait", 500],
    ["face", "thinking"],
    ["wait", 500],
    ["face", "happy"],
    ["wait", 500]
  ]
}
```

This proved that a Skill can combine:

```text
direct function
+ local wait
+ reusable recipe
+ direct function
```

The executor resolves `recipe` locally and translates approved physical calls such as `face` into Runtime API requests.

Relevant implementation commit:

- `8b68a02863651105bd9814c979b86379cc4f65d4` — minimal JrSkill JSON v1 with local Recipe execution.

### Physical validation

The complete Skill was physically tested on the JrBot.

Observed behavior:

```text
happy
  ↓
wait 500 ms
  ↓
recipe face_sequence
    ├── surprised
    ├── thinking
    └── happy
  ↓
neutral
```

The face changes were visually confirmed on the physical OLED.

The executor completed with:

```text
JR_SKILL_V1 result=ok
```

### Day 2 result

The following pieces are now physically validated together:

- minimal JrSkill JSON v1;
- direct function calls;
- local `wait`;
- local `recipe` resolution;
- reusable Recipe blocks;
- Runtime API capability execution;
- observable behavior on physical JrBot hardware.

### Decision at the end of Day 2

The JrSkill JSON v1 is now **frozen for the next proof**.

No additional functions or schema complexity will be added before the first blockchain test.

The next development stage is intentionally postponed to **2026-09-30**.

Planned next proof:

```text
validated JrSkill JSON
        ↓
Solana Devnet
        ↓
Skill PDA
        ↓
retrieve the same JSON
        ↓
validate integrity
        ↓
existing JrSkill Executor
        ↓
physical JrBot
```

No Solana implementation was started on Day 2.

---

## Day 2 checkpoint (historical)

At the end of 2026-09-29:

```text
LOCAL SKILL FORMAT        validated
LOCAL EXECUTOR            validated
LOCAL RECIPE              validated
RUNTIME API PATH          validated
PHYSICAL OLED EXECUTION   validated
SOLANA SKILL PDA          next stage
LICENSE PDA               later stage
```

The objective for the next development day is not to expand JrSkill. It is to prove that the already validated Skill can cross the blockchain boundary unchanged.

---

## 2026-09-30 — Day 3: Stage 2 isolated Solana implementation

**Day 3 CLOSED — our first JrSkill smart contract is deployed on Solana Devnet, and the original Skill JSON has been published and independently recovered with identical bytes.**

Today's milestone completes **Stage 2**, the isolated blockchain storage/retrieval proof. The physical local executor was already validated on Day 2; blockchain-driven robot execution remains a later stage. The day ends at this checkpoint, with documentation and public evidence ready to share.

Work performed exclusively on `V1s-00`, starting from `6fee26ec0e1168207ce245c41731db0ca7ac9859`, after checking the remote branch and Issue #33.

### Implemented

- Minimal single-file Anchor workspace at `solana/jrskill-solana` (Anchor 1.1.2).
- `create_skill` instruction and immutable Skill PDA derived from `["skill", authority, SHA-256(payload)]`.
- Account fields: `authority`, `schema_version`, `payload_hash`, `payload`.
- Signer/payer requirement, schema version 1, hash validation and a 512-byte payload limit.
- Devnet publication and independent read scripts, with genesis-hash guard and byte-for-byte verification.
- Payload/IDL tests and local-validator tests for successful creation/read and invalid inputs.
- [Stage 2 architecture and test plan](JRSKILL_SOLANA_STAGE2.md), with complete WSL commands in the workspace README.

The existing JSON v1 Git blob is unchanged (128 LF bytes). The Windows checkout used in local tests has CRLF (137 bytes); both hashes and their implications are recorded in the Stage 2 document. Neither the Recipe nor the robot executor was changed.

Technical stack: Anchor CLI/crate/client **1.1.2**, Solana CLI **3.1.10**, `@solana/web3.js` **1.98.4**, Rust `sha2` **0.10.9**, with Cargo/npm lockfiles. The user's WSL Node runtime was **24.10.0**; the standalone public proof requires Node **20+**. Program package version: **0.1.0**. The storage instruction has no update/delete/close path; the program itself remains upgradeable by the recorded wallet authority. JSON/schema interpretation stays in the client; the contract validates the storage envelope, length and hash rather than executing Skill calls.

### Evidence executed

- Rust host check completed (`cargo check`).
- Anchor IDL generation completed.
- Anchor SBF build completed, producing `jrskill.so` and IDL.
- Three payload tests and two generated-IDL tests passed.
- One on-chain test passed against a local WSL validator, including creation/read, duplicate rejection, hash mismatch, unsupported version, empty/oversized payload and incorrect seeds.
- The local validator loaded the program with `--bpf-program`; the JavaScript client ran on Windows. The integrated `anchor test` command was not run because WSL lacked Node/npm.

### Initial checkpoint (historical)

At this initial checkpoint, Stage 2 was **implemented, compiled and exercised on a local validator**, with no Devnet deploy or payload round-trip yet. No firmware build or physical robot test was part of this work.

### Subsequent user WSL validation — 2026-09-30

User-provided logs confirmed successful installation, 3 payload tests and 2 IDL tests. The first integrated test exposed two environment/configuration issues: Anchor 1.1.2 defaults to Surfpool, and `anchor keys sync` updates only the selected cluster. The guide now explicitly selects `--validator legacy` and syncs both devnet and localnet with the existing program key.

After these corrections, the user ran the complete integrated test:

```bash
anchor test --validator legacy --provider.cluster localnet --provider.wallet target/local-test-wallet.json
```

Result: **1 test passed, 0 failed**, including exact payload round-trip and rejected invalid inputs (500.476268 ms). Compilation profiles also completed successfully. Local program public key: `Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454`; this is not evidence of a Devnet deploy. The initial cross-environment test remains recorded above as historical evidence.

### Subsequent Devnet program deploy — 2026-09-30

The user rebuilt for devnet, passed the 2 IDL tests and deployed the program. The log confirmed `Deploy success` and IDL metadata initialization (`2nKon1p32sHB7DWLrpcxh5x6z5s864KYM6RZ675baeaa`). Independent Devnet RPC inspection confirmed:

- Program ID: `Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454`;
- upgrade authority: `3Sce1sfA6q2m2mNr2VhyGfoTePa3vA9WjYYq2JYA5mij`;
- last deployed slot: **505968344**;
- program data length: **191976 bytes**.

The supplied deploy log did not include a transaction signature. At this intermediate checkpoint, the program was deployed on Devnet while payload publication and independent retrieval were still pending. Those remaining checks were completed later in the same day, as recorded below.

### Client network guard correction — 2026-09-30

The user's first `publish:devnet` attempt stopped before sending a transaction. The Devnet genesis-hash constant was truncated. Read-only queries to the official RPC confirmed the full hash `EtWTRABZaYq6iMfeYKouRu166VU2xqa1wcaWoxPkrZBG`. The client now requires the complete hash and reports the received genesis on rejection. Two regression tests were added; all 5 payload/network tests and a live read-only Devnet guard check passed in the agent environment. No program change or redeploy was required, and no Skill publication/read was proven by this correction.

### Stage 2 completed — Devnet payload round-trip, 2026-09-30

User-provided logs confirmed successful publication (`already_exists: false`) followed by an independent read command with `byte_equal: true`.

- Program: `Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454`.
- Authority: `3Sce1sfA6q2m2mNr2VhyGfoTePa3vA9WjYYq2JYA5mij`.
- Skill PDA: `8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux`.
- Publication signature: `4MJJ8awPvv4LKjHHemodRbSKAwMgcJaUZRyudNP81ZzVGUao8cHXLskGadF5VyjVwNh4iKQw8dLNn9frnMthogC6`.
- Payload: **128 bytes**, SHA-256 `416d6af34eada998a5f46595e0355a5bdfd7dba86faefe312dbc2afcd26d907f`, matching the original Git blob.
- Transaction: **410 bytes**; Skill account: **589 bytes**; reported rent: **3642360 lamports**.
- Independent retrieval: **`result: "ok"`, `byte_equal: true`**.

**Stage 2 is validated on Devnet.** The unchanged v1 JSON crossed the chain boundary and was recovered exactly. The deployed public Program ID is now pinned in source/config; private keys remain local. Repeat publication is still an optional unproven check. No robot integration, Recipe publication, licensing or blockchain-driven physical test was performed.

### Public reproducible proof — read the Skill without a wallet

Anyone can independently retrieve the frozen Stage 2 JSON from Solana Devnet. A standalone verifier is available at `solana/jrskill-solana/scripts/verify-public.mjs`. It needs only **Node.js 20+** and outbound HTTPS access to the official Devnet RPC. It does not require a wallet, private key, Solana CLI, Anchor, npm packages, local IDL or local Skill JSON.

On Linux/macOS/WSL, download the verifier and execute it in any working directory:

```bash
curl -fsSL https://raw.githubusercontent.com/JuniorNarciso26/JrRobot/V1s-00/solana/jrskill-solana/scripts/verify-public.mjs -o jrskill-verify.mjs
node jrskill-verify.mjs
```

Inside an existing checkout, simply run:

```bash
node solana/jrskill-solana/scripts/verify-public.mjs
```

**The verifier comes from GitHub; the payload comes from the Solana RPC.** The script checks the Devnet genesis, reads the public Skill PDA at `finalized` commitment, checks the owner, Anchor discriminator, publisher authority, schema and payload bounds, decodes the frozen Borsh layout, recomputes SHA-256 and compares it with the published checkpoint hash. It never signs or sends a transaction and never executes Skill calls.

Expected proof fields (the `rpc_slot` varies with the time of reading):

```json
{
  "result": "ok",
  "source": "solana-devnet-rpc",
  "payload_bytes": 128,
  "payload_hash": "416d6af34eada998a5f46595e0355a5bdfd7dba86faefe312dbc2afcd26d907f",
  "hash_verified": true,
  "matches_checkpoint": true
}
```

The output then prints the JSON recovered from the account:

```json
{
  "v": 1,
  "run": [
    ["face", "happy"],
    ["wait", 500],
    ["recipe", "face_sequence"],
    ["face", "neutral"]
  ]
}
```

This verifier targets the frozen Stage 2 account layout and expected hash; it is not a generic Skill explorer. RPC availability/rate limits can temporarily prevent a read. The original `read:devnet` command still requires the workspace, generated IDL and local reference JSON for its byte-for-byte comparison.

Live read-only verification succeeded with Node on Windows and Node on Linux/WSL, including execution from `/tmp` outside the workspace. Both returned 128 bytes, the checkpoint hash, `hash_verified: true`, `matches_checkpoint: true` and the original JSON.

### Day 3 closing technical checkpoint

The completed proof is:

```text
Existing JrSkill JSON v1 (128 original bytes)
    ↓ client validation + SHA-256
create_skill → authority signature → Solana Devnet
    ↓
Skill PDA stores authority + schema_version + payload_hash + payload
    ↓ independent finalized RPC read + Borsh decoding
Original JSON recovered with identical bytes and matching hash
    ↓
Public read-only verifier reproduces the proof without a wallet
```

Final evidence levels:

| Component | Day 3 result |
| --- | --- |
| Anchor program and clients | Implemented |
| Host check, SBF compilation and IDL generation | Completed successfully |
| Payload/network tests | **5 passed** |
| Generated-IDL tests | **2 passed** |
| Integrated local-validator suite | **1 passed**, covering creation/read, duplicate, hash, version, empty/oversized payload and incorrect PDA rejection |
| Devnet program and IDL deploy | Confirmed |
| Skill publication transaction | **Finalized**, confirmed independently with `solana confirm --url devnet` |
| Independent Skill retrieval | **128 bytes**, original SHA-256, **`byte_equal: true`** |
| Public standalone proof | Passed on Windows and Linux/WSL, including download from GitHub and execution outside the workspace |
| Blockchain-driven JrBot execution | Not implemented or physically tested on Day 3 |
| License PDA / access control | Deferred |

The three test suites contain **8 passing tests in total**; this count does not include IDL-generation internals or the separate public RPC verification runs. There was no firmware build, new robot capability, new Recipe or physical test on this day. The JSON v1 Git blob remained `ecb748e1bbce58f381e6be0cda68725b2f7ae73d`.

Three concrete issues were diagnosed and corrected before closing:

1. **Validator selection:** Anchor defaults to Surfpool; the documented local flow now uses `--validator legacy` with the installed Solana test validator.
2. **Program IDs per cluster:** `anchor keys sync` updates only the selected cluster; devnet/localnet are explicitly synced with the same existing key, and the deployed public ID is now pinned in source/config.
3. **Devnet guard:** the initially truncated genesis hash was replaced by the full official value, with regression coverage. The network check remained mandatory.

Dependency audit notices were recorded; no forced dependency migration was performed during this isolated proof. The Explorer currently reports the program as upgradeable and without a verified build/security.txt. Functional validation on Devnet does not claim reproducible source-to-bytecode verification or a security audit. These remain separate follow-up topics.

Public evidence:

- [Deployed JrSkill program](https://explorer.solana.com/address/Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454?cluster=devnet)
- [Skill PDA containing the JSON](https://explorer.solana.com/address/8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux?cluster=devnet)
- [Finalized Skill publication transaction](https://explorer.solana.com/tx/4MJJ8awPvv4LKjHHemodRbSKAwMgcJaUZRyudNP81ZzVGUao8cHXLskGadF5VyjVwNh4iKQw8dLNn9frnMthogC6?cluster=devnet)
- [Program source](../solana/jrskill-solana/programs/jrskill/src/lib.rs)
- [Standalone public verifier](../solana/jrskill-solana/scripts/verify-public.mjs)
- [Architecture and full Stage 2 test plan](JRSKILL_SOLANA_STAGE2.md)

### Day 3 implementation and validation commits

All work stayed in **V1s-00**. No branch was created and no changes were pushed to `main`, `develop`, `v1` or `v2`.

| Commit | Technical checkpoint |
| --- | --- |
| [18f5ca9](https://github.com/JuniorNarciso26/JrRobot/commit/18f5ca9e789b7ad13ff0a76f91a74d3f32698c1f) | Anchor workspace, Skill PDA, Devnet clients and initial tests |
| [94ffb1f](https://github.com/JuniorNarciso26/JrRobot/commit/94ffb1fd87c52a294b8aadcadc4ebd50d84a77b4) | Architecture and initial local validation record |
| [3680ead](https://github.com/JuniorNarciso26/JrRobot/commit/3680ead0a9edc309b74526b4a094fe2f77333cd2) | Legacy validator command correction |
| [6c02cf5](https://github.com/JuniorNarciso26/JrRobot/commit/6c02cf5bb00890ac9ced84870b2b382d3b0c5398) | Explicit Program ID sync for both clusters |
| [7b4edab](https://github.com/JuniorNarciso26/JrRobot/commit/7b4edab6b19b7d6417023d39458f33a8dd71961a) | Successful integrated WSL test checkpoint |
| [445f197](https://github.com/JuniorNarciso26/JrRobot/commit/445f19768f71edbd5050db4c0650030f291bf0a7) | Confirmed Devnet program deploy |
| [dc35061](https://github.com/JuniorNarciso26/JrRobot/commit/dc35061ababbb399e4e339b23578073cf75f9410) | Complete Devnet genesis hash and regression tests |
| [045eb5b](https://github.com/JuniorNarciso26/JrRobot/commit/045eb5b34363c1f18b8b29a2cff918b4b5227ac0) | Validated Devnet round-trip and pinned public Program ID |
| [7071724](https://github.com/JuniorNarciso26/JrRobot/commit/707172499249c9b2de91b93bd00450c79bcbaa6a) | Standalone public proof and reproduction instructions |

### Colosseum update — ready-to-publish draft

**Day 3 — Our first JrSkill smart contract is live on Solana Devnet!**

Today we completed the first isolated blockchain proof for JrSkill Network: we deployed our Anchor program, published the same declarative JrSkill JSON already validated locally, and recovered it from Solana Devnet with identical bytes.

The program creates a Skill PDA containing the publisher authority, schema version, SHA-256 hash and payload. The address is derived from the publisher and content hash. The contract verifies the signer, supported version, payload size and hash; it does not execute robot actions.

We passed 8 tests across payload/network validation, the generated IDL and a local-validator suite, then completed the Devnet deploy and publication. The Skill publication transaction is finalized. An independent read recovered all 128 original bytes, with the same SHA-256 and `byte_equal: true`.

Anyone can reproduce this proof with our standalone Node.js verifier: no wallet, private key, Anchor or local Skill file is required. The verifier reads the public account directly from Solana and prints the recovered JSON.

JrBot predates the hackathon; today's work is the new Solana storage/retrieval layer. Robot execution from blockchain data and licensing are later stages. Our Day 3 milestone is complete: **the original Skill JSON is on-chain and publicly retrievable.**

[Program](https://explorer.solana.com/address/Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454?cluster=devnet) · [Skill account](https://explorer.solana.com/address/8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux?cluster=devnet) · [Publication transaction](https://explorer.solana.com/tx/4MJJ8awPvv4LKjHHemodRbSKAwMgcJaUZRyudNP81ZzVGUao8cHXLskGadF5VyjVwNh4iKQw8dLNn9frnMthogC6?cluster=devnet) · [Public proof and full development log](https://github.com/JuniorNarciso26/JrRobot/blob/V1s-00/docs/HACKATHON_DEVLOG.md#public-reproducible-proof--read-the-skill-without-a-wallet)

### After Day 3 (not started)

The next development day can study Stage 3 (Solana → existing executor → Runtime API → JrBot) before implementing integration. Stage 4 licensing remains deferred. No further implementation is part of this Day 3 closure.

---

## References

- [JrSkill Execution API](JRSKILL_API.md)
- [Issue #33 — JrSkill Network / Solana Hackathon 2026](https://github.com/JuniorNarciso26/JrRobot/issues/33)
- [Experimental branch: V1s-00](https://github.com/JuniorNarciso26/JrRobot/tree/V1s-00)
- [Colosseum project page](https://colosseum.com/arena/projects/jrskill-network)
