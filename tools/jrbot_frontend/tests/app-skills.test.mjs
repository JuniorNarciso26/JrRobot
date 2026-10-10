import test from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import {readFileSync} from 'node:fs';
const source=readFileSync(new URL('../../../firmware/main/jrskill_app.js',import.meta.url),'utf8');
async function simulate(doc){
 const commands=[], elements=new Map();
 const context=vm.createContext({document:{readyState:'loading',addEventListener(){},getElementById(id){if(!elements.has(id))elements.set(id,{textContent:'',style:{},disabled:false});return elements.get(id);}},commands,doc,setTimeout:fn=>fn(),console});
 const expose=`globalThis.testRun=async()=>{selected='selected';authenticated=true;address='buyer';networkOk=true;discover=async()=>[{skill:'selected',doc}];fresh=async()=>{};command=async obj=>{commands.push(obj);return {result:obj.fn==='capabilities'?{actions:['face']}:{expression:obj.args.expression}};};await run();};`;
 vm.runInContext(source.replace('})();',expose+'})();'),context);
 await vm.runInContext('testRun()',context);
 return {commands,message:elements.get('jrskill-app-msg').textContent};
}
test('App executes the new licensed Skill with sad and worried',async()=>{
 const r=await simulate({v:1,run:[['face','happy'],['wait',500],['face','sad'],['face','thinking'],['face','neutral'],['face','worried']]});
 assert.deepEqual(r.commands.filter(x=>x.fn==='face').map(x=>x.args.expression),['happy','sad','thinking','neutral','worried']);
 assert.match(r.message,/result=ok/);
});
test('App rejects unsupported late face or schema before any runtime command',async()=>{
 for(const doc of [{v:1,run:[['face','happy'],['face','invalid']]},{v:2,run:[['face','happy']]}]){
  const r=await simulate(doc);assert.equal(r.commands.length,0);assert.match(r.message,/interrompida/);
 }
});

test('App accepts the 16 canonical OLED expressions',async()=>{
 const names=['neutral','happy','sad','excited','angry','surprised','thinking','skeptical','sleepy','confused','winking','love','playful','worried','cool','battery_low'];
 const r=await simulate({v:1,run:names.map(name=>['face',name])});
 assert.deepEqual(r.commands.filter(x=>x.fn==='face').map(x=>x.args.expression),names);
 assert.match(r.message,/result=ok/);
});
