'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
const source=fs.readFileSync(path.join(__dirname,'../../tools/jrbot_frontend/panel.js'),'utf8').split('for(const [name,command] of faces)')[0];
const elements=new Map();
function element(id){if(!elements.has(id))elements.set(id,{textContent:'',value:id==='audio_volume'?'10':'',children:[],classList:{toggle(){}},appendChild(n){this.children.push(n)},removeChild(){this.children.shift()}});return elements.get(id);}
let requests=0;
const ctx=vm.createContext({console,TextEncoder,URLSearchParams,AbortController,Date,setTimeout:()=>0,clearTimeout:()=>{},
 document:{getElementById:element,querySelectorAll:()=>[],createElement:()=>({})},
 fetch:async()=>{requests++;return {ok:true,text:async()=>'JR_OK audio_test=tx_completed'};},
 localStorage:{setItem(){},getItem(){return null;}}});
vm.runInContext(source,ctx);
const render=(version,audio)=>vm.runInContext(`renderStatus('JR_STATUS protocol=2 version=${version} hardware=JRBOT-HW-04 profile=headless_diagnostic oled=disabled audio=${audio} camera=disabled mic=pinout_defined')`,ctx);
(async()=>{
 let passed=0;
 render('JRBOT-V2-DIAG-02','on_demand');assert.equal(element('test_audio').disabled,true);passed++;
 await assert.rejects(vm.runInContext("send('audio_test')",ctx));assert.equal(requests,0);passed++;
 render('JRBOT-V2-DIAG-03','disabled');assert.equal(element('test_audio').disabled,true);passed++;
 render('JRBOT-V2-DIAG-03','on_demand');assert.equal(element('test_audio').disabled,false);passed++;
 await vm.runInContext("send('audio_test')",ctx);assert.equal(requests,1);passed++;
 assert.match(element('mic_state').textContent,/MS3625/);passed++;
 assert.equal(element('test_mic').disabled,true);passed++;
 console.log(JSON.stringify({scope:'Node + minimal DOM mocks only',passed,failed:0}));
})().catch(e=>{console.error(e);process.exitCode=1;});
