import test from 'node:test';
import assert from 'node:assert/strict';
import { createSkillSearch } from '../src/skills.mjs';
const buyer = '6rinyiBQyS4RxLeuX62yJqxgwiJqWXiyAbxXSS5LX85R';
const wallet = { address: buyer, active: {}, pending: false };
const auth = { address: buyer, authenticated: true };
const proof = { cluster: 'devnet', genesis: 'EtWTRABZaYq6iMfeYKouRu166VU2xqa1wcaWoxPkrZBG',
  program: 'Ax11PmTRcz3NLBSxtLm38Aush3MY5GJoBjyjggjtS454', buyer, commitment: 'finalized', rpc_slot: 1,
  skills: [{ name: 'minimal_recipe_01', skill: '8LRRfZVnyjSYPLezJBCdGriwVcbzsopogZBDTAFSFJux',
    license: '7tPf4YSd7P6PzBkmseW5FrwnG8P238gZTVG8Rj2v9v45', model_version: 0, schema_version: 1,
    hash_verified: true, payload_hash: 'a'.repeat(64), checkpoint_supported: true }] };
const tick = () => new Promise(resolve => setImmediate(resolve));
test('authentication starts discovery; manual refresh and empty list work', async () => {
  let state, calls = 0;
  const search = createSkillSearch(async () => { calls++; return calls === 1 ? proof : {...proof, skills: []}; }, s => state = s);
  search.observe(wallet, {authenticated: false}); assert.equal(calls, 0);
  search.observe(wallet, auth); await tick(); assert.equal(state.skills.length, 1);
  await search.search(); assert.equal(state.skills.length, 0); assert.match(state.message, /nao possui/);
});
test('wallet changes discard late discovery and clear prior results', async () => {
  let state, resolve;
  const search = createSkillSearch(() => new Promise(r => resolve = r), s => state = s);
  search.observe(wallet, auth);
  search.observe({address: '', active: undefined}, {authenticated: false});
  resolve(proof); await tick(); assert.equal(state.skills.length, 0); assert.equal(state.authenticated, false);
});
test('wrong buyer/network, duplicate rows and RPC failure never become an empty-success result', async () => {
  for (const response of [{...proof, buyer: 'other'}, {...proof, genesis: 'mainnet'}, {...proof, skills: [proof.skills[0], proof.skills[0]]}, null]) {
    let state;
    const search = createSkillSearch(async () => { if (!response) throw new Error('RPC offline'); return response; }, s => state = s);
    search.observe(wallet, auth); await tick();
    assert.equal(state.skills.length, 0); assert.match(state.message, /Busca nao confirmada/);
  }
});
