import test from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import { readFileSync } from 'node:fs';

const source = readFileSync(new URL('../panel.js', import.meta.url), 'utf8').split('async function sendCustom()')[0];
const payload = readFileSync(new URL('../../../docs/HACKATHON_DEVLOG/assets/day3/recovered-skill.json', import.meta.url), 'utf8');
const recipe = readFileSync(new URL('../jrskill/recipes/face_sequence.json', import.meta.url), 'utf8');

async function simulate(failure) {
  const logs = [], faces = [], reads = [], elements = new Map();
  const context = vm.createContext({console, TextEncoder, setTimeout: fn => { fn(); return 1; }, clearTimeout(){},
    document: {getElementById(id){if(!elements.has(id)) elements.set(id,{textContent:'',value:''}); return elements.get(id);}},
    logs, capturedFaces: faces, reads, payload, recipe, failure});
  vm.runInContext(source, context);
  vm.runInContext(`
    mode='serial'; serialConnected=true; devnetState='available';
    currentFirmware=()=>true; oledAvailable=()=>true; refreshControls=()=>{};
    localLine=line=>logs.push(line);
    api=async path=>{
      reads.push(path);
      if(path==='/jrskill/devnet-status')return JSON.stringify({available:failure!=='offline',cluster:'devnet'});
      if(path==='/jrskill/skill')return payload;
      if(path==='/jrskill/skill-devnet'){
        if(failure==='rpc')throw new Error('RPC offline');
        return JSON.stringify({source:'solana-devnet',commitment:'finalized',hash_verified:failure!=='hash',matches_checkpoint:true,payload_text:payload,pda:'checkpoint',rpc_slot:123,payload_bytes:128,payload_hash:'verified'});
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
  assert.deepEqual(result.reads,['/jrskill/skill-devnet','/jrskill/recipe/face_sequence']);
  assert.ok(result.logs.includes('JR_SKILL_V1 start=v1 source=solana-devnet top_calls=4'));
  assert.ok(result.logs.includes('JR_SKILL_V1 result=ok source=solana-devnet'));
});
test('RPC failure, rejected proof and absent capability send no face commands', async()=>{
  for(const failure of ['rpc','hash','capability']){
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
