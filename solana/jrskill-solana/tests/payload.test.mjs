import test from 'node:test';
import assert from 'node:assert/strict';
import { mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { loadPayload, validatePayload, verifySkill, sha256, SOURCE_URL } from '../scripts/payload.mjs';

test('loads the existing v1 file without serialization or Recipe expansion', () => {
  const original = loadPayload();
  assert.deepEqual(original.payload, readFileSync(SOURCE_URL));
  assert.deepEqual(JSON.parse(original.payload), { v: 1, run: [['face', 'happy'], ['wait', 500], ['recipe', 'face_sequence'], ['face', 'neutral']] });
  assert.deepEqual(original.payloadHash, sha256(original.payload));
});

test('loads an arbitrary JrSkill v1 file passed by path', () => {
  const dir = mkdtempSync(join(tmpdir(), 'jrskill-payload-'));
  try {
    const path = join(dir, 'second-skill.json');
    const bytes = Buffer.from('{"v":1,"run":[["face","love"],["wait",750],["face","neutral"]]}\n');
    writeFileSync(path, bytes);
    const original = loadPayload(path);
    assert.deepEqual(original.payload, bytes);
    assert.deepEqual(original.payloadHash, sha256(bytes));
    assert.equal(original.schemaVersion, 1);
    assert.equal(original.source, path);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test('round-trip verification rejects tampered bytes, metadata and hash', () => {
  const original = loadPayload();
  const authority = { toBase58: () => 'publisher' };
  const account = { authority, schemaVersion: 1, payloadHash: original.payloadHash, payload: original.payload };
  assert.deepEqual(verifySkill(account, authority, original), original.payload);
  assert.throws(() => verifySkill({ ...account, payload: Buffer.from('{}') }, authority, original));
  assert.throws(() => verifySkill({ ...account, payloadHash: Buffer.alloc(32) }, authority, original));
  assert.throws(() => verifySkill({ ...account, schemaVersion: 2 }, authority, original));
  assert.throws(() => verifySkill({ ...account, authority: { toBase58: () => 'other' } }, authority, original));
});

test('client rejects invalid v1 and oversized payloads', () => {
  for (const invalid of ['', '{', '{"v":2,"run":[]}', '{"v":1,"run":[["shell","x"]]}', '{"v":1,"run":[],"license":true}']) {
    assert.throws(() => validatePayload(Buffer.from(invalid)));
  }
  assert.throws(() => validatePayload(Buffer.alloc(513)));
});
