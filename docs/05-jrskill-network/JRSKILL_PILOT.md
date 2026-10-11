# JrSkill External Pilot — Build a Skill and See It Run on a Robot

The internal pilot was completed on **October 7, 2026**: `jrteste` was created through the Store, purchased with a different wallet, and executed using the JrBot App and development panel.

We are now seeking the first **three external contributors**. This is a target, **not** a claim that three external contributors have already joined.

You can participate with or without a JrBot. Contributors without hardware can request a physical test from the maintainer. The entry point is [GitHub invitation #53](https://github.com/JuniorNarciso26/JrRobot/issues/53).

## Path A — Publish through the Store

1. Open [Create Your Skill](https://jrbot.com.br/en/store/create/) in a browser with Phantom.
2. Connect and authenticate your wallet by signing a message.
3. Select Solana **Devnet** and request test SOL through the [faucet](https://faucet.solana.com/).
4. Enter a name, description, price in test SOL, and an original sequence of three to eight expressions.
5. For this first pilot, use `happy`, `sad`, `surprised`, `thinking`, `worried`, and `neutral`, which were exercised in physical tests. Prefer waits of 500–1000 ms and end with `neutral`.
6. Prepare the quotation, review costs, accept the terms, and sign publication in Phantom. Wait for confirmation and save the Skill and transaction URLs.
7. Check the Skill in the [Store catalog](https://jrbot.com.br/en/store/) and post its link in issue #53.

**Limits:** JSON v1, payload up to 512 bytes, up to 32 editor steps, and integer waits of 0–5000 ms. Payload size may limit the sequence before 32 steps. This pilot uses expressions and waits only, without external recipes.

When you publish using your wallet, it becomes the on-chain creator. The price and creator/JrBot allocation appear in the purchase quotation. All transactions use **test SOL on Devnet**.

## Path B — Contribute without a wallet or your own publication

Send an original JSON, Skill name, description, and preferred attribution in issue #53:

```json
{"v":1,"run":[["face","happy"],["wait",500],["face","worried"],["wait",500],["face","neutral"]]}
```

This JSON is an example; submit your own sequence. The maintainer can review it and publish from a maintainer wallet. In that case, **the maintainer's publishing wallet is the on-chain creator**, while your authorship is credited in the submission and demonstration. Choose Path A if your wallet must be the on-chain creator.

## Testing with or without a robot

**With a JrBot:** use `V1s-00`, firmware `JrBot_V1S_APP_03` or a documented successor supporting the pilot, and panel `JRBOT-PANEL-V1S-SKILLS-15`. Purchase the license using a different buyer wallet. In the App or panel, authenticate → verify Devnet → discover Skills → select → execute. Submit the exact version, result, logs, and a video if available.

**Without a JrBot:** once published, the maintainer can acquire/verify the license using a test wallet and execute the Skill on physical hardware. You do not need to purchase a license merely to submit an idea. The maintainer reports approval, failure, or required changes and records physical evidence when available.

## Submission and tracking

In [issue #53](https://github.com/JuniorNarciso26/JrRobot/issues/53), provide the Skill name and URL/PDA or assisted-publication JSON, intended behavior, whether you own a JrBot, preferred credit, permission to publish a demonstration, and results or difficulties when applicable.

Each contribution follows this lifecycle:

```text
Received → Validated → Published → Licensed → Physical test → Evidence and feedback
```

Each step must be confirmed separately; on-chain publication does not by itself establish successful physical execution.

After the first external contributor, the team will refine onboarding based on feedback and proceed with the next two.

## Shareable invitation

> Build a JrSkill and see your idea run on a real robot! We are looking for the first external contributors. Create an original OLED expression sequence, publish it on Solana Devnet, or submit JSON for assisted publication. No robot required: we can test your Skill on a physical JrBot and share the results with your chosen attribution. Test SOL only.
>
> Join: https://github.com/JuniorNarciso26/JrRobot/issues/53

History: [issue #52](https://github.com/JuniorNarciso26/JrRobot/issues/52). Implementation details: [JrSkill App](JRSKILL_APP.md).
