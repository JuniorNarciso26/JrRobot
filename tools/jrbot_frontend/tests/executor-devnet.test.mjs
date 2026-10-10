import test from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import { readFileSync } from 'node:fs';

const source = readFileSync(new URL('../panel.js', import.meta.url), 'utf8').split('async function sendCustom()')[0];
const payload = readFileSync(new URL('../../../docs/HACKATHON_DEVLOG/assets/day3/recovered-skill.json', import.meta.url), 'utf8');
const recipe = readFileSync(new URL('../jrskill/recipes/face_sequence.json', import.meta.url), 'utf8');

async function simulate(failure) {
  const logs = [], faces = [], reads = [], elements = new Map();
  const context = vm.createContext({console, TextEncoder, URLSearchParams, setTimeout: fn => { fn(); return 1; }, clearTimeout(){},
    document: {getElementById(id){if(!elements.has(id)) elements.set(id,{textContent:'',value:''}); return elements.get(id);}},
    logs, capturedFaces: faces, reads, payload: failure === 'new-skill' ? JSON.stringify({v:1,run:[['face','happy'],['face','sad'],['face','worried']]}) : payload, recipe, failure});
  vm.runInContext(source, context);
  vm.runInContext(`
    el('jrskill_selected').value=failure==='new-skill'?'selected-new':'checkpoint';
    mode='serial'; serialConnected=true; devnetState='available';
    currentFirmware=()=>true; oledAvailable=()=>true; refreshControls=()=>{};
    updateSkillWallet('buyer',failure!=='unauth','mock');
    localLine=line=>logs.push(line);
    api=async (path,options)=>{
      reads.push(path);
      if(path==='/jrskill/devnet-status')return JSON.stringify({available:failure!=='offline',cluster:'devnet'});
      if(path==='/jrskill/skill')return payload;
      if(path==='/jrskill/wallet/execution/start'){
        if(failure==='rpc')throw new Error('RPC offline');
        if(failure==='unlicensed')throw new Error('execution_license_absent');
        if(failure==='late-account')updateSkillWallet('other',false,'mock');
        return JSON.stringify({buyer:'buyer',execution_permit:'permit',license:'license',source:'solana-devnet',commitment:'finalized',hash_verified:failure!=='hash',matches_checkpoint:failure!=='new-skill',payload_text:payload,pda:failure==='wrong-skill'?'other':options.body.get('skill'),rpc_slot:123,payload_bytes:128,payload_hash:'verified'});
      }
      if(path==='/jrskill/wallet/execution/send'){
        const command=options.body.get('command');
        if(failure==='midrun'&&capturedFaces.length===1)throw new Error('RPC offline');
        const reply=await send(command);
        if(failure==='account-change'&&capturedFaces.length===1)updateSkillWallet('other',false,'mock');
        return JSON.stringify({reply});
      }
      if(path==='/jrskill/recipe/face_sequence')return recipe;
      throw new Error('Unexpected read '+path);
    };
    send=async command=>{
      const call=JSON.parse(command.slice(4));
      if(call.fn==='capabilities')return 'JR_API '+JSON.stringify({ok:true,result:{actions:failure==='capability'?[]:['face']}});
      capturedFaces.push(call.args.expression);
      return 'JR_API '+JSON.stringify({ok:true,result:{expression:call.args.expression}});
    };
  `, context);
  if(failure==='offline'||failure==='reconnect'){
    await vm.runInContext("checkDevnetConnection()", context);
    if(failure==='offline')await vm.runInContext("runJrSkillTest()", context);
  }else await vm.runInContext("runJrSkillTest('solana-devnet')", context);
  return {logs, faces, reads, devnetState:vm.runInContext('devnetState',context)};
}

test('Devnet Skill uses existing executor/Recipe without reading local Skill', async()=>{
  const result=await simulate();
  assert.deepEqual(result.faces,['happy','surprised','thinking','happy','neutral']);
  assert.ok(result.reads.includes('/jrskill/wallet/execution/start'));
  assert.equal(result.reads.filter(path=>path==='/jrskill/wallet/execution/send').length,6);
  assert.ok(result.reads.includes('/jrskill/recipe/face_sequence'));
  assert.ok(result.logs.includes('JR_SKILL_V1 start=v1 source=solana-devnet top_calls=4'));
  assert.ok(result.logs.includes('JR_SKILL_V1 result=ok source=solana-devnet'));
});

test('unlicensed, unauthenticated and late wallet changes dispatch no faces',async()=>{
  for(const failure of ['unlicensed','unauth','late-account']){
    const result=await simulate(failure);assert.deepEqual(result.faces,[]);
    assert.ok(result.logs.some(line=>line.startsWith('JR_SKILL_AUTHORIZATION result=blocked')));
  }
});
test('wallet change and RPC failure stop subsequent physical commands',async()=>{
  for(const failure of ['account-change','midrun']){
    const result=await simulate(failure);assert.deepEqual(result.faces,['happy']);
    assert.ok(!result.logs.includes('JR_SKILL_V1 result=ok source=solana-devnet'));
  }
});
test('RPC failure, rejected proof and absent capability send no face commands', async()=>{
  for(const failure of ['rpc','hash','capability','wrong-skill']){
    const result=await simulate(failure);
    assert.deepEqual(result.faces,[]);
    assert.ok(!result.reads.includes('/jrskill/skill'));
    assert.ok(result.logs.some(line=>line.startsWith('JR_SKILL_V1 result=error source=solana-devnet')));
  }
});
test('offline status keeps local execution working; successful probe restores availability',async()=>{
  const offline=await simulate('offline');
  assert.equal(offline.devnetState,'unavailable');
  assert.deepEqual(offline.faces,['happy','surprised','thinking','happy','neutral']);
  assert.ok(!offline.reads.includes('/jrskill/skill-devnet'));
  const online=await simulate('reconnect');
  assert.equal(online.devnetState,'available');
  assert.deepEqual(online.faces,[]);
});

test('panel executes selected new Skill without requiring reference checkpoint',async()=>{
 const result=await simulate('new-skill');
 assert.deepEqual(result.faces,['happy','sad','worried']);
 assert.ok(result.logs.some(line=>line.includes('pda=selected-new')));
 assert.ok(result.logs.includes('JR_SKILL_V1 result=ok source=solana-devnet'));
});
