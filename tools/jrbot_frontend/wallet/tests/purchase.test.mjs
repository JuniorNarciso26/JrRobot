import test from 'node:test';
import assert from 'node:assert/strict';
import { createPurchase } from '../src/purchase.mjs';
const fixture = () => {
  let state, now = 100, signerCalls = [], calls = [], logs = [], handler;
  const account = { address: 'buyer', chains: ['solana:devnet'], features: ['solana:signTransaction'] };
  const wallet = { features: { 'solana:signTransaction': { supportedTransactionVersions: ['legacy'], signTransaction: async input => {
    signerCalls.push(input); return [{ signedTransaction: new Uint8Array(417) }];
  } } } };
  const proof = { cluster: 'devnet', genesis: 'EtWTRABZaYq6iMfeYKouRu166VU2xqa1wcaWoxPkrZBG', program: 'Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454',
    skill: '8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux', buyer: 'buyer', commitment: 'finalized', owned: false, model_version: 0,
    price_lamports: '1000000000', creator_lamports: '500000000', treasury_lamports: '500000000', license: 'license',
    quote_id: 'quote', transaction: btoa('transaction'), expires_at: 190, rent_lamports: '1346200', fee_lamports: '5000', total_lamports: '1001351200',
    priority_fee_limit_lamports: '100000', fee_limit_lamports: '105000', total_limit_lamports: '1001451200' };
  const api = createPurchase(async (op, data) => { calls.push(op); return handler ? handler(op, data) : op === 'submit' ?
    { buyer: 'buyer', cluster: 'devnet', signature: '2'.repeat(88), state: 'submitted', fee_lamports: '80000', fee_limit_lamports: '105000' } : { ...proof }; }, next => state = next, line => logs.push(line), () => now);
  const observe = (address = 'buyer', authenticated = true) => api.observe({ active: wallet, accounts: [{ ...account, address }], address }, { authenticated });
  observe();
  return { api, observe, calls, signerCalls, wallet, proof, logs, get state() { return state; }, time: value => now = value, handle: fn => handler = fn };
};
test('quote never signs, explicit acceptance signs Devnet exactly once and waits for license', async () => {
  const f = fixture(); await f.api.prepare(); await f.api.buy(); assert.equal(f.signerCalls.length, 0);
  f.api.accept(true); await f.api.buy(); assert.equal(f.signerCalls.length, 1);
  assert.equal(f.signerCalls[0].chain, 'solana:devnet'); assert.equal(f.state.owned, false);
  await f.api.buy(); assert.equal(f.signerCalls.length, 1);
  f.handle(() => ({ ...f.proof, owned: true })); await f.api.check(); assert.equal(f.state.owned, true);
});
test('refusal and expired quote never submit', async () => {
  const f = fixture(); await f.api.prepare(); f.api.accept(true); f.time(191); await f.api.buy(); assert.equal(f.signerCalls.length, 0);
  f.time(100); await f.api.prepare(); f.api.accept(true);
  f.wallet.features['solana:signTransaction'].signTransaction = async () => { throw Object.assign(new Error('refused'), { code: 4001 }); };
  await f.api.buy(); assert.ok(!f.calls.includes('submit')); assert.match(f.state.message, /recusada/);
});
test('account/session change discards late quote or signature before relay', async () => {
  const f = fixture(); let finish;
  f.handle(() => new Promise(resolve => finish = resolve));
  const waiting = f.api.prepare(); f.observe('other', false); finish(f.proof); await waiting; assert.equal(f.state.quote, null);
  f.observe(); f.handle(null); await f.api.prepare(); f.api.accept(true);
  f.wallet.features['solana:signTransaction'].signTransaction = () => new Promise(resolve => finish = resolve);
  const buying = f.api.buy(); f.observe('other', false); finish([{ signedTransaction: new Uint8Array(417) }]); await buying;
  assert.ok(!f.calls.includes('submit')); assert.equal(f.state.signature, '');
});
test('owned, mismatched and ambiguous submission responses cannot cause repeat payment', async () => {
  const f = fixture(); f.handle(() => ({ ...f.proof, owned: true })); await f.api.prepare(); assert.equal(f.state.canBuy, false);
  f.observe('other', true); f.handle(() => f.proof); await f.api.prepare(); assert.equal(f.state.quote, null);
  f.observe(); f.handle(null); await f.api.prepare(); f.api.accept(true);
  f.handle(() => { throw new Error('timeout'); }); await f.api.buy(); assert.equal(f.state.signature, 'unknown');
  await f.api.prepare(); await f.api.buy(); assert.equal(f.signerCalls.length, 1);
});
test('RPC uncertainty preserves signature without claiming a successful send', async () => {
  const f = fixture(); await f.api.prepare(); f.api.accept(true);
  f.handle(() => ({ buyer: 'buyer', cluster: 'devnet', signature: '2'.repeat(88), state: 'unknown' }));
  await f.api.buy(); assert.match(f.state.message, /Envio nao confirmado/);
  assert.equal(f.state.owned, false); assert.equal(f.state.canBuy, false);
});

test('HTTP rejection is visible and logged; subsequent license check never claims a send', async () => {
  const f = fixture(); await f.api.prepare(); f.api.accept(true);
  f.handle(() => { throw new Error('HTTP 400: Cotacao usada/expirada ou transacao alterada'); });
  await f.api.buy(); assert.match(f.state.message, /HTTP 400: Cotacao/);
  assert.ok(f.logs.some(line => /stage=submit_http state=error/.test(line) && /HTTP 400/.test(line)));
  f.handle(null); await f.api.check(); assert.match(f.state.message, /Envio nao confirmado/);
  assert.doesNotMatch(f.state.message, /Compra enviada|RPC recebeu/);
  assert.equal(f.state.canQuote, false); assert.equal(f.state.canBuy, false);
});

test('RPC diagnostic and signing failure are retained without logging transaction bytes', async () => {
  const f = fixture(); await f.api.prepare(); f.api.accept(true);
  f.handle(() => ({ buyer: 'buyer', cluster: 'devnet', signature: '2'.repeat(88), state: 'unknown', error_detail: 'RPC code=-32002 message=Blockhash not found' }));
  await f.api.buy(); assert.match(f.state.message, /Blockhash not found/);
  assert.ok(f.logs.some(line => /stage=rpc_relay/.test(line) && /code=-32002/.test(line)));
  f.handle(null); await f.api.check(); assert.doesNotMatch(f.state.message, /Compra enviada|RPC recebeu/);
  f.observe('other', false); f.observe(); await f.api.prepare(); f.api.accept(true);
  f.wallet.features['solana:signTransaction'].signTransaction = async () => { throw new Error('refused\n' + 'A'.repeat(200)); };
  await f.api.buy(); assert.ok(f.logs.some(line => /stage=wallet_signature state=error/.test(line)));
  assert.ok(f.logs.every(line => !line.includes('A'.repeat(100)) && !line.includes('\n')));
});

test('quote requires an explicit consistent fee ceiling and rejects altered limits before signing', async () => {
  const f = fixture();
  f.handle(() => ({ ...f.proof, fee_limit_lamports: '999999' }));
  await f.api.prepare(); assert.equal(f.state.quote, null); assert.equal(f.state.canBuy, false);
  f.handle(null); await f.api.prepare(); assert.equal(f.state.quote.total_limit_lamports, '1001451200');
  f.api.accept(true);
  f.handle(() => ({ buyer: 'buyer', cluster: 'devnet', signature: '2'.repeat(88), state: 'submitted', fee_lamports: '105001', fee_limit_lamports: '105000' }));
  await f.api.buy(); assert.match(f.state.message, /teto aceito/); assert.equal(f.state.canBuy, false);
});
