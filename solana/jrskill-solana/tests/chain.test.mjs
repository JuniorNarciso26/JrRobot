import test from 'node:test';
import assert from 'node:assert/strict';
import { connect, deriveSkill, createInstruction, readVerified, Transaction, SystemProgram } from '../scripts/client.mjs';
import { loadPayload, sha256 } from '../scripts/payload.mjs';

test('local validator: exact payload, immutable PDA and rejected invalid inputs', async () => {
  const { program, provider, connection } = await connect({ local: true });
  const authority = provider.wallet.publicKey;
  const original = loadPayload();
  const pda = deriveSkill(program.programId, authority, original.payloadHash);
  await provider.sendAndConfirm(new Transaction().add(await createInstruction(program, authority, original)));
  const { recovered } = await readVerified(program, authority, original);
  assert.deepEqual(recovered, original.payload);
  await assert.rejects(async () => provider.sendAndConfirm(new Transaction().add(await createInstruction(program, authority, original))));

  async function rejected(schema, payload, hash, code) {
    const skill = deriveSkill(program.programId, authority, hash);
    await assert.rejects(() => program.methods.createSkill(schema, [...hash], payload)
      .accountsStrict({ skill, authority, systemProgram: SystemProgram.programId }).rpc(),
      (error) => error.error?.errorCode?.code === code);
    assert.equal(await connection.getAccountInfo(skill), null, 'Failed init must roll back');
  }
  await rejected(1, original.payload, Buffer.alloc(32, 1), 'HashMismatch');
  const schemaPayload = Buffer.from('{"v":2,"run":[]}');
  await rejected(2, schemaPayload, sha256(schemaPayload), 'UnsupportedSchema');
  await rejected(1, Buffer.alloc(0), sha256(Buffer.alloc(0)), 'EmptyPayload');
  const oversized = Buffer.alloc(513, 32);
  await rejected(1, oversized, sha256(oversized), 'PayloadTooLarge');
  const wrongPda = deriveSkill(program.programId, authority, Buffer.alloc(32, 2));
  await assert.rejects(() => program.methods.createSkill(1, [...original.payloadHash], original.payload)
    .accountsStrict({ skill: wrongPda, authority, systemProgram: SystemProgram.programId }).rpc(),
    (error) => error.error?.errorCode?.code === 'ConstraintSeeds');
  assert.equal(await connection.getAccountInfo(wrongPda), null);
});
