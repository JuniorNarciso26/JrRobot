// Run after anchor build (or anchor idl build). No RPC, wallet or deployment.
import test from 'node:test';
import assert from 'node:assert/strict';
import { createRequire } from 'node:module';
import { readFileSync } from 'node:fs';
import { createInstruction, deriveSkill, PublicKey, Transaction } from '../scripts/client.mjs';
import { loadPayload, verifySkill } from '../scripts/payload.mjs';

const anchor = createRequire(import.meta.url)('@anchor-lang/core');
const idl = JSON.parse(readFileSync(new URL('../target/idl/jrskill.json', import.meta.url), 'utf8'));
const program = new anchor.Program(idl, { connection: new anchor.web3.Connection('http://127.0.0.1:8899') });

test('generated IDL encodes the instruction within the transaction limit', async () => {
  const authority = anchor.web3.Keypair.generate().publicKey;
  const original = loadPayload();
  const instruction = await createInstruction(program, authority, original);
  const tx = new Transaction().add(instruction);
  tx.feePayer = authority;
  tx.recentBlockhash = PublicKey.default.toBase58();
  assert.ok(tx.serialize({ requireAllSignatures: false, verifySignatures: false }).length <= 1232);
  const decoded = program.coder.instruction.decode(instruction.data);
  assert.equal(decoded.name, 'createSkill');
  assert.deepEqual(Buffer.from(decoded.data.payload), original.payload);
  assert.equal(instruction.keys.find((key) => key.pubkey.equals(authority)).isSigner, true);
  assert.ok(instruction.keys[0].pubkey.equals(deriveSkill(program.programId, authority, original.payloadHash)));
  assert.ok(!deriveSkill(program.programId, authority, original.payloadHash).equals(
    deriveSkill(program.programId, anchor.web3.Keypair.generate().publicKey, original.payloadHash)));
});

test('generated IDL account codec preserves all fields and checks discriminator', async () => {
  const authority = anchor.web3.Keypair.generate().publicKey;
  const original = loadPayload();
  const account = { authority, schemaVersion: 1, payloadHash: [...original.payloadHash], payload: original.payload };
  const encoded = await program.coder.accounts.encode('skill', account);
  const decoded = program.coder.accounts.decode('skill', encoded);
  assert.deepEqual(verifySkill(decoded, authority, original), original.payload);
  encoded[0] ^= 1;
  assert.throws(() => program.coder.accounts.decode('skill', encoded));
});
