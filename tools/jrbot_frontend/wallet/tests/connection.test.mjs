import test from 'node:test';
import assert from 'node:assert/strict';
import { createWalletController } from '../src/controller.mjs';

const account = address => ({ address, chains: ['solana:devnet'], publicKey: new Uint8Array(32) });
function fixture() {
  const registryEvents = {}, logs = [], calls = [];
  const wallets = [];
  let state;
  const registry = {
    get: () => wallets,
    on: (event, cb) => { registryEvents[event] = cb; }
  };
  function wallet(name, initial = [account('A')]) {
    let event;
    const value = { name, chains: ['solana:devnet'], accounts: initial, features: {
      'standard:connect': { connect: async () => { calls.push('connect:' + name); return { accounts: value.accounts }; } },
      'standard:disconnect': { disconnect: async () => { calls.push('disconnect:' + name); } },
      'standard:events': { on: (_, cb) => { event = cb; return () => { event = null; }; } },
      'solana:signMessage': { signMessage: () => assert.fail('Connection must never sign') },
      'solana:signTransaction': { signTransaction: () => assert.fail('Connection must never transact') }
    } };
    value.change = next => { value.accounts = next; event?.({ accounts: next }); };
    return value;
  }
  const controller = createWalletController(registry, next => { state = next; }, line => logs.push(line));
  return { controller, calls, logs, wallet, get state() { return state; },
    add(value) { wallets.push(value); registryEvents.register(value); },
    remove(value) { wallets.splice(wallets.indexOf(value), 1); registryEvents.unregister(value); }
  };
}

test('late discovery lists only Devnet-capable Wallet Standard extensions; never auto-connects', () => {
  const f = fixture();
  assert.equal(f.state.wallets.length, 0);
  const good = f.wallet('Phantom'); f.add(good);
  const evm = f.wallet('EVM'); evm.chains = ['eip155:1']; f.add(evm);
  assert.deepEqual(f.state.wallets, [good]);
  assert.deepEqual(f.calls, []);
});

test('explicit connection, account selection, account change and revocation clear stale address', async () => {
  const f = fixture(), w = f.wallet('Phantom', [account('A'), account('B')]); f.add(w);
  await f.controller.connect(w);
  assert.equal(f.state.address, 'A');
  assert.equal(f.state.pending, false);
  f.controller.selectAccount('B'); assert.equal(f.state.address, 'B');
  f.controller.selectAccount('unapproved'); assert.equal(f.state.address, 'B');
  w.change([account('C')]); assert.equal(f.state.address, 'C');
  w.change([]); assert.equal(f.state.address, '');
  assert.ok(f.logs.every(line => line.includes('authenticated=false')));
});

test('switching provider disconnects old extension and ignores its later events', async () => {
  const f = fixture(), first = f.wallet('One'), second = f.wallet('Two', [account('B')]);
  f.add(first); f.add(second);
  await f.controller.connect(first); await f.controller.connect(second);
  first.change([account('old')]);
  assert.equal(f.state.address, 'B');
  assert.deepEqual(f.calls, ['connect:One', 'disconnect:One', 'connect:Two']);
  await f.controller.disconnect();
  assert.equal(f.state.active, null); assert.equal(f.state.address, '');
});

test('rejection leaves no connected account and can retry', async () => {
  const f = fixture(), w = f.wallet('One'); f.add(w);
  const connect = w.features['standard:connect'].connect;
  w.features['standard:connect'].connect = async () => { throw Object.assign(new Error('rejected'), { code: 4001 }); };
  await f.controller.connect(w);
  assert.equal(f.state.address, ''); assert.equal(f.state.pending, false);
  assert.match(f.state.message, /recusada/);
  w.features['standard:connect'].connect = connect;
  await f.controller.connect(w); assert.equal(f.state.address, 'A');
});

test('disconnect error still clears all local account state', async () => {
  const f = fixture(), w = f.wallet('One'); f.add(w); await f.controller.connect(w);
  w.features['standard:disconnect'].disconnect = async () => { throw new Error('unavailable'); };
  await f.controller.disconnect();
  assert.equal(f.state.address, ''); assert.equal(f.state.active, null);
  assert.equal(f.state.pending, false); assert.match(f.state.message, /Revogue/);
});

test('removed extension cannot finish an in-flight connection; duplicate clicks do not request twice', async () => {
  const f = fixture(), w = f.wallet('One'); f.add(w);
  let resolve, requests = 0;
  w.features['standard:connect'].connect = () => { requests++; return new Promise(done => { resolve = done; }); };
  const attempt = f.controller.connect(w);
  await f.controller.connect(w); assert.equal(requests, 1);
  f.remove(w); resolve({ accounts: [account('stale')] }); await attempt;
  assert.equal(f.state.address, ''); assert.equal(f.state.active, null);
  assert.equal(f.state.pending, false);
});

test('connected extension removal and unsupported authorized account clear connection display', async () => {
  const f = fixture(), w = f.wallet('One', [{ address: 'EVM', chains: ['eip155:1'] }]); f.add(w);
  await f.controller.connect(w); assert.equal(f.state.address, '');
  w.change([account('A')]); assert.equal(f.state.address, 'A');
  f.remove(w); assert.equal(f.state.address, ''); assert.equal(f.state.active, null);
});
