# Build your first JrSkill

**External pilot — create a Skill and see it run on a real robot.**

The internal `jrteste` pilot completed creation, catalogue, purchase by another
wallet, licensing and physical execution in the JrBot App and development panel
on October 7, 2026. We are now looking for the first **3 external contributors**.
No external participation is claimed yet.

You can join with or without a robot. Start or submit your contribution in
[the invitation, issue #53](https://github.com/JuniorNarciso26/JrRobot/issues/53).
[Portuguese step-by-step guide](JRSKILL_PILOT.md).

## Publish through the Store

1. Open [Create your Skill](https://jrbot.com.br/en/store/create/) with Phantom.
2. Connect and authenticate your wallet by signing the message.
3. Use Solana Devnet and get test SOL from [the faucet](https://faucet.solana.com/).
4. Enter a name, description, test SOL price and an original sequence of 3–8 expressions.
5. For this first round use `happy`, `sad`, `surprised`, `thinking`, `worried`,
   and `neutral`, covered by physical tests. Prefer waits of 500–1000 ms and a neutral ending.
6. Prepare the quote, review costs, explicitly accept and sign publication in Phantom.
   Wait for confirmation and save the Skill/transaction links.
7. Check the [catalogue](https://jrbot.com.br/en/store/) and reply in issue #53 with your link/PDA.

Limits: JrSkill JSON v1, 512 payload bytes, up to 32 face/wait steps in the editor,
integer waits of 0–5000 ms. Byte size can limit the sequence before 32 steps.
This pilot uses faces and waits; do not submit external recipes or executable code.
Your publishing wallet becomes the on-chain creator. All publishing and purchasing
use test SOL on Devnet.

## Submit without a wallet

Reply in issue #53 with an original JSON, name, intended behavior and attribution:

```json
{"v":1,"run":[["face","happy"],["wait",500],["face","worried"],["wait",500],["face","neutral"]]}
```

This is an example; create your own sequence. The maintainer can review and publish
it on your behalf. In assisted publication the maintainer's publishing wallet is
the on-chain creator; your contribution is credited in the submission and demo.
Use the Store path if you want your own wallet to be the on-chain creator.

## Physical testing

**Have a JrBot?** Use `V1s-00`, firmware `JrBot_V1S_APP_03` or a documented successor,
and panel `JRBOT-PANEL-V1S-SKILLS-15`. Use a different buyer wallet to purchase the
license in the Store. In the App or panel: authenticate → verify Devnet → discover
Skills → select → execute. Send the exact version, result, log and optional video.

**No robot?** The maintainer can license the published Skill with a test wallet,
run it on the physical JrBot and record the result. You do not need to purchase
a license just to submit a contribution.

## Submission and follow-up

Include your Skill link/PDA or JSON, expected behavior, hardware availability,
chosen attribution, and feedback about unclear steps. State whether you permit
publication of a demo with your credit. Do not post private keys or seed phrases.

We track each contribution separately: received → validated → published → licensed
→ physical test → evidence and feedback. Publication alone is not physical validation.
The maintainer reports failures as well as successful runs.

After the first contributor, we revise onboarding using their feedback before
continuing with the other two. The internal maintainer pilot is not external traction.

References: [technical tracking #33](https://github.com/JuniorNarciso26/JrRobot/issues/33),
[traction tracking #52](https://github.com/JuniorNarciso26/JrRobot/issues/52),
[execution API](JRSKILL_API.md), [App and panel](JRSKILL_APP.md).
