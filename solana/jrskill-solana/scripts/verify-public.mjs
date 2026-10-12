// Standalone, read-only proof of the frozen Stage 2 Devnet checkpoint.
// Requires Node.js 20+ and HTTPS access; no npm packages, wallet or local IDL.
import { createHash } from 'node:crypto';

const RPC = 'https://api.devnet.solana.com';
const GENESIS = 'EtWTRABZaYq6iMfeYKouRu166VU2xqa1wcaWoxPkrZBG';
const PROGRAM = 'Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454';
const PDA = '8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux';
const AUTHORITY = '3Sce1sfA6q2m2mNr2VhyGfoTePa3vA9WjYYq2JYA5mij';
const AUTHORITY_BYTES = Buffer.from('244730b82bf46825305434825d7413b132296edc9145b2374ab278f9bdb5eef4', 'hex');
const EXPECTED_HASH = '416d6af34eada998a5f46595e0355a5bdfd7dba86faefe312dbc2afcd26d907f';
const digest = (bytes) => createHash('sha256').update(bytes).digest();

async function rpc(method, params = []) {
  const response = await fetch(RPC, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ jsonrpc: '2.0', id: 1, method, params }),
    signal: AbortSignal.timeout(20000),
  });
  if (!response.ok) throw new Error(`RPC HTTP ${response.status}`);
  const body = await response.json();
  if (body.error) throw new Error(`RPC error: ${JSON.stringify(body.error)}`);
  if (!Object.hasOwn(body, 'result')) throw new Error('Missing RPC result');
  return body.result;
}

try {
  if (await rpc('getGenesisHash') !== GENESIS) throw new Error('Devnet genesis mismatch');
  const result = await rpc('getAccountInfo', [PDA, { encoding: 'base64', commitment: 'finalized' }]);
  const account = result.value;
  if (!account || account.owner !== PROGRAM || account.executable) throw new Error('Missing Skill or invalid owner/type');
  if (!Array.isArray(account.data) || account.data[1] !== 'base64') throw new Error('Unexpected RPC encoding');
  const data = Buffer.from(account.data[0], 'base64');
  // Anchor/Borsh: discriminator(8), authority(32), schema(1), hash(32), length(u32 LE), payload.
  if (data.length < 77 || !data.subarray(0, 8).equals(digest('account:Skill').subarray(0, 8))) {
    throw new Error('Invalid Skill discriminator or truncated data');
  }
  if (!data.subarray(8, 40).equals(AUTHORITY_BYTES)) throw new Error('Authority mismatch');
  if (data[40] !== 1) throw new Error('Unsupported schema version');
  const length = data.readUInt32LE(73);
  if (length < 1 || length > 512 || 77 + length > data.length) throw new Error('Invalid payload length');
  const payload = data.subarray(77, 77 + length);
  const hash = digest(payload);
  if (!data.subarray(41, 73).equals(hash)) throw new Error('Stored payload hash mismatch');
  if (length !== 128 || hash.toString('hex') !== EXPECTED_HASH) throw new Error('Payload differs from frozen Stage 2 checkpoint');
  const text = new TextDecoder('utf-8', { fatal: true }).decode(payload);
  const json = JSON.parse(text);
  if (json.v !== 1 || !Array.isArray(json.run)) throw new Error('Invalid JrSkill v1 JSON');
  console.log(JSON.stringify({
    result: 'ok', source: 'solana-devnet-rpc', commitment: 'finalized', rpc_slot: result.context.slot,
    program: PROGRAM, authority: AUTHORITY, pda: PDA, payload_bytes: length,
    payload_hash: hash.toString('hex'), hash_verified: true, matches_checkpoint: true,
  }));
  process.stdout.write(text);
} catch (error) {
  console.error(error.message);
  process.exitCode = 1;
}
