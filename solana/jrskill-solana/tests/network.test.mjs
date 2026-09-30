import test from 'node:test';
import assert from 'node:assert/strict';
import { assertDevnetGenesis, DEVNET_GENESIS, PublicKey } from '../scripts/client.mjs';

test('accepts the complete genesis hash returned by the official Devnet RPC', () => {
  // Captured with solana genesis-hash --url https://api.devnet.solana.com (2026-09-30).
  const rpcGenesis = 'EtWTRABZaYq6iMfeYKouRu166VU2xqa1wcaWoxPkrZBG';
  assert.doesNotThrow(() => assertDevnetGenesis(rpcGenesis));
  assert.equal(new PublicKey(DEVNET_GENESIS).toBuffer().length, 32);
});

test('rejects another network, missing values and the previously truncated hash', () => {
  for (const genesis of [PublicKey.default.toBase58(), undefined, '', 'EtWTRABZaYq6iMfeYKouRu166VU2xqa1']) {
    assert.throws(() => assertDevnetGenesis(genesis), /Devnet required; refusing another cluster/);
  }
});
