import test from 'node:test';
import assert from 'node:assert/strict';
import { createAuthenticator } from '../src/auth.mjs';

function fixture() {
  const calls = [], logs = [];
  let state, now = 1000, verifyResult = { authenticated: true, address: 'A', expires_at: 1600 };
  const account = { address: 'A' };
  const wallet = { features: { 'solana:signMessage': { signMessage: async ({ message }) =>
    [{ signedMessage: message, signature: new Uint8Array(64) }] } } };
  const auth = createAuthenticator(async (operation, data) => {
    calls.push({ operation, data });
    if (operation === 'challenge') return { id: 'nonce', message: 'test challenge' };
    return operation === 'logout' ? { authenticated: false } : verifyResult;
  }, value => { state = value; }, line => logs.push(line), () => now);
  const observe = (address = 'A') => auth.observe({ active: address ? wallet : null,
    accounts: address ? [{ ...account, address }] : [], address });
  observe();
  return { auth, observe, wallet, calls, logs, get state() { return state; },
    now(value) { now = value; }, result(value) { verifyResult = value; } };
}

test('authentication is explicit, verifies with server, expires and resets on account change', async () => {
  const f = fixture(); assert.equal(f.state.authenticated, false);
  await f.auth.authenticate(); assert.equal(f.state.authenticated, true);
  assert.deepEqual(f.calls.map(item => item.operation), ['logout', 'challenge', 'verify']);
  f.observe('B'); assert.equal(f.state.authenticated, false);
  f.observe('A'); await f.auth.authenticate(); assert.equal(f.state.authenticated, true);
  f.now(1600); await f.auth.check(); assert.equal(f.state.authenticated, false);
});

test('refusal and altered signed bytes never verify; connection can retry', async () => {
  const f = fixture(), method = f.wallet.features['solana:signMessage'];
  const original = method.signMessage;
  method.signMessage = async () => { throw Object.assign(new Error('denied'), { code: 4001 }); };
  await f.auth.authenticate(); assert.equal(f.state.authenticated, false);
  assert.ok(!f.calls.some(item => item.operation === 'verify'));
  method.signMessage = async () => [{ signedMessage: new Uint8Array(), signature: new Uint8Array(64) }];
  await f.auth.authenticate(); assert.equal(f.state.authenticated, false);
  assert.ok(!f.calls.some(item => item.operation === 'verify'));
  method.signMessage = original; await f.auth.authenticate(); assert.equal(f.state.authenticated, true);
});

test('account change while signing discards late signature and never authenticates new account', async () => {
  const f = fixture(); let finish;
  f.wallet.features['solana:signMessage'].signMessage = ({ message }) =>
    new Promise(resolve => { finish = () => resolve([{ signedMessage: message, signature: new Uint8Array(64) }]); });
  const signing = f.auth.authenticate();
  while (!finish) await new Promise(resolve => setImmediate(resolve));
  f.observe('B'); finish(); await signing;
  assert.equal(f.state.authenticated, false);
  assert.ok(!f.calls.some(item => item.operation === 'verify'));
});

test('another address from verifier and server revocation fail closed', async () => {
  const f = fixture(); f.result({ authenticated: true, address: 'B', expires_at: 1600 });
  await f.auth.authenticate(); assert.equal(f.state.authenticated, false);
  f.result({ authenticated: true, address: 'A', expires_at: 1600 });
  await f.auth.authenticate(); assert.equal(f.state.authenticated, true);
  f.result({ authenticated: false }); await f.auth.check(); assert.equal(f.state.authenticated, false);
  f.observe(''); assert.equal(f.state.canSign, false);
});
