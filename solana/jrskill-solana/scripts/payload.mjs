import { createHash } from 'node:crypto';
import { readFileSync } from 'node:fs';

export const SOURCE_URL = new URL('../../../tools/jrbot_frontend/jrskill/skills/minimal_recipe_01.json', import.meta.url);
export const MAX_PAYLOAD_BYTES = 512;
export const sha256 = (bytes) => createHash('sha256').update(bytes).digest();

export function validatePayload(bytes) {
  if (!bytes.length || bytes.length > MAX_PAYLOAD_BYTES) throw new Error('Payload length must be 1..512 bytes');
  const json = JSON.parse(new TextDecoder('utf-8', { fatal: true }).decode(bytes));
  if (json.v !== 1 || !Array.isArray(json.run) || Object.keys(json).sort().join(',') !== 'run,v') {
    throw new Error('Expected frozen JrSkill v1 {v, run}');
  }
  for (const call of json.run) {
    if (!Array.isArray(call) || call.length !== 2) throw new Error('Invalid v1 call');
    const [fn, arg] = call;
    if (fn === 'wait' ? !Number.isSafeInteger(arg) || arg < 0
      : !['face', 'recipe'].includes(fn) || typeof arg !== 'string' || !arg.length) {
      throw new Error('Unsupported v1 call');
    }
  }
  return json;
}

export function loadPayload() {
  const payload = readFileSync(SOURCE_URL); // Never stringify, normalize or expand Recipes.
  const json = validatePayload(payload);
  return { payload, payloadHash: sha256(payload), schemaVersion: json.v };
}

export function verifySkill(account, authority, original) {
  const recovered = Buffer.from(account.payload);
  if (account.authority.toBase58() !== authority.toBase58()) throw new Error('Authority mismatch');
  if (account.schemaVersion !== original.schemaVersion) throw new Error('Schema mismatch');
  if (!Buffer.from(account.payloadHash).equals(sha256(recovered))) throw new Error('Stored hash mismatch');
  if (!Buffer.from(account.payloadHash).equals(original.payloadHash)) throw new Error('Source hash mismatch');
  if (!recovered.equals(original.payload)) throw new Error('Byte-for-byte comparison failed');
  validatePayload(recovered);
  return recovered;
}
