import test from 'node:test';
import assert from 'node:assert/strict';
import { createNetworkCheck } from '../src/network.mjs';

const connected = address => ({ address, active: { chains: ['solana:devnet'] }, accounts: [{ address, chains: ['solana:devnet'] }] });
const proof = address => ({ address, cluster: 'devnet', rpc_verified: true,
  genesis: 'EtWTRABZaYq6iMfeYKouRu166VU2xqa1wcaWoxPkrZBG', commitment: 'finalized',
  balance_lamports: '0', balance_sol: '0', rpc_slot: 1 });
const tick = () => new Promise(resolve => setImmediate(resolve));

test('only authenticated wallet triggers check; zero balance is a successful RPC read', async () => {
  let state, calls = 0;
  const check = createNetworkCheck(async () => { calls++; return proof('A'); }, value => { state = value; });
  check.observe(connected('A'), { authenticated: false }); await check.check(); assert.equal(calls, 0);
  check.observe(connected('A'), { authenticated: true }); await tick();
  assert.equal(calls, 1); assert.equal(state.verified, true); assert.equal(state.balance, '0');
  assert.match(state.message, /nao confirma a rede selecionada/);
  check.observe(connected('A'), { authenticated: false }); assert.equal(state.verified, false);
});

test('switching address drops late proof, and mismatch/offline never display zero as verified', async () => {
  let state, finish;
  const check = createNetworkCheck(() => new Promise(resolve => { finish = resolve; }), value => { state = value; });
  check.observe(connected('A'), { authenticated: true });
  check.observe(connected('B'), { authenticated: false }); finish(proof('A')); await tick();
  assert.equal(state.verified, false); assert.equal(state.balance, '');
  for (const response of [() => proof('B'), () => { throw new Error('offline'); }, () => ({ ...proof('A'), genesis: 'mainnet' })]) {
    const next = createNetworkCheck(async () => response(), value => { state = value; });
    next.observe(connected('A'), { authenticated: true }); await tick();
    assert.equal(state.verified, false); assert.equal(state.balance, '');
  }
});
