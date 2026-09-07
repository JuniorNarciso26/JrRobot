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
const render=(version,audio,mic='available')=>vm.runInContext(`renderStatus('JR_STATUS protocol=2 version=${version} build_sp=2026-09-06_07:10_BRT hardware=JRBOT-HW-04 profile=full_hardware_test oled=ready oled_presence=available audio=${audio} camera=available camera_pid=0x5640 mic=${mic} mic_channel=L mic_peak_raw=1000')`,ctx);
(async()=>{
 let passed=0;
 render('JRBOT-V2-DIAG-03','on_demand');assert.equal(element('test_audio').disabled,true);passed++;
 await assert.rejects(vm.runInContext("send('audio_test')",ctx));assert.equal(requests,0);passed++;
 render('JRBotV2_2026-09-06-07:10','on_demand');assert.equal(element('test_audio').disabled,false);passed++;
 assert.equal(element('test_camera').disabled,false);passed++;
 assert.equal(element('test_mic').disabled,false);passed++;
 render('JRBotV2_2026-09-06-07:10','on_demand','unavailable');assert.equal(element('test_mic').disabled,true);passed++;
 console.log(JSON.stringify({scope:'Node + minimal DOM mocks only',passed,failed:0}));
})().catch(e=>{console.error(e);process.exitCode=1;});
