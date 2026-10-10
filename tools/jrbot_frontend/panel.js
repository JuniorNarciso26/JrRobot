'use strict';
const FIRMWARE_PREFIXES=['JrBot_V1.','JrBot_V1S_','JRBotV2_'];
const faces=[['Neutro','neutro'],['Feliz','feliz'],['Triste','triste'],['Animado','animado'],['Bravo','bravo'],['Surpreso','surpreso'],['Pensando','pensando'],['Cetico','cetico'],['Sono','sono'],['Confuso','confuso'],['Piscando','piscando'],['Amor','amor'],['Brincalhao','brincalhao'],['Preocupado','preocupado'],['Cool','cool'],['Bateria','bateria']];
const el=id=>document.getElementById(id), val=id=>el(id).value.trim();
const logEl=el('log');
let serverCursor=0, serverSession='', autoScroll=true, busy=false, device=null;
let mode='serial', serialConnected=false, activePort='';
let devnetState='unknown', devnetChecking=false;
let skillWallet={address:'',authenticated:false,provider:null,revision:0};
function updateSkillWallet(address,authenticated,provider){if(skillWallet.address!==address||skillWallet.authenticated!==authenticated||skillWallet.provider!==provider)skillWallet={address,authenticated,provider,revision:skillWallet.revision+1};refreshControls();}
function appendLog(items){for(const item of items){const line=document.createElement('div');line.textContent='['+item.ts+'] '+item.line;logEl.appendChild(line);el('last').textContent=item.ts;}while(logEl.children.length>1200)logEl.removeChild(logEl.firstChild);if(autoScroll)logEl.scrollTop=logEl.scrollHeight;}
function localLine(line){appendLog([{ts:new Date().toLocaleTimeString(),line}]);}
function clearLog(){logEl.textContent='';}
function downloadLog(){const blob=new Blob([Array.from(logEl.children,n=>n.textContent).join('\n')],{type:'text/plain;charset=utf-8'});const url=URL.createObjectURL(blob),a=document.createElement('a');a.href=url;a.download='JrBot-painel-log.txt';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}
function openWifiConfig(){el('wifi_config_title').scrollIntoView({behavior:'smooth'});}
function connection(text,ok){el('conn').textContent=text;el('conn').className='pill '+(ok?'ok':'bad');}
function fields(text){const result={};for(const token of text.trim().split(/\s+/)){const at=token.indexOf('=');if(at>0)result[token.slice(0,at)]=token.slice(at+1);}return result;}
function currentFirmware(){return !!device&&typeof device.version==='string'&&FIRMWARE_PREFIXES.some(prefix=>device.version.startsWith(prefix))&&device.hardware==='JRBOT-HW-04';}
function oledAvailable(){return !!device&&device.oled_presence==='available'&&['ready','degraded'].includes(device.oled);}
function refreshControls(){document.querySelectorAll('[data-action]').forEach(b=>b.disabled=busy);const valid=currentFirmware();el('test_audio').disabled=busy||!valid||!['ready','on_demand'].includes(device?.audio);el('test_camera').disabled=busy||!valid||mode!=='serial'||device?.camera!=='available';el('test_mic').disabled=busy||!valid||device?.mic!=='available';document.querySelectorAll('#faces button,#demo').forEach(b=>b.disabled=busy||!valid||!oledAvailable());el('audio_volume').disabled=busy||!valid;el('apply_volume').disabled=busy||!valid;el('jrskill_devnet_test').disabled=busy||devnetState!=='available'||!skillWallet.authenticated;el('jrskill_devnet_check').disabled=busy||devnetChecking;}
function invalidate(message){device=null;el('device_msg').textContent=message;for(const id of ['fw_version','fw_profile','oled_state','audio_state','camera_state','mic_state'])el(id).textContent='Nao verificado';refreshControls();}
function renderStatus(text){if(!text.startsWith('JR_STATUS protocol=2 ')){invalidate('Firmware sem protocolo V2.');connection('Firmware nao confirmado',false);throw new Error('Firmware sem protocolo V2.');}const next=fields(text);if(!next.version||!next.profile||!next.hardware){invalidate('Estado incompleto.');connection('Firmware nao confirmado',false);throw new Error('Estado incompleto do firmware.');}device=next;const valid=currentFirmware();el('face_controls').open=oledAvailable();el('fw_version').textContent=next.version;el('fw_profile').textContent=next.profile;el('oled_state').textContent=next.oled_presence==='available'?'Disponivel - '+next.oled:'Indisponivel - '+(next.oled||'offline');const audioSeq=Number(next.audio_test_seq||0),audioBytes=Number(next.audio_last_bytes||0);el('audio_state').textContent=!valid?'Firmware antigo/incompativel':next.audio_test_running==='1'?'Teste em andamento':audioSeq>0?'Ultimo teste #'+audioSeq+' - '+(next.audio_last||'?')+' - '+audioBytes+' bytes':'Pronto para teste - MAX98357A';el('camera_state').textContent=next.camera==='available'?'Disponivel - OV5640 '+(next.camera_pid||''):'Indisponivel - OV5640';el('mic_state').textContent=next.mic==='available'?'Disponivel - MS3625 canal '+(next.mic_channel||'?')+' pico '+(next.mic_peak_raw||'0'):'Indisponivel - MS3625';if(/^\d+$/.test(next.volume||'')){el('audio_volume').value=next.volume;el('audioVolText').textContent=next.volume+'%';}el('device_msg').textContent='Estado atualizado. Build: '+(next.build_sp||next.version)+'.'+(next.pending_restart==='1'?' Wi-Fi pendente de reinicializacao.':'');el('audio_msg').textContent=audioSeq>0?'Diagnostico do audio: teste #'+audioSeq+', resultado '+(next.audio_last||'?')+', '+audioBytes+' bytes transmitidos. Se ESP_OK e nao houve som, confira MAX98357A/falante.':'MAX98357A: LRC GPIO47, BCLK GPIO21, DIN GPIO42. O teste usa o volume ja aplicado e envia um unico comando.';el('cam_msg').textContent=next.camera==='available'?'OV5640 detectada. Teste liberado.':'OV5640 nao respondeu. Conecte e clique Atualizar estado.';el('face_msg').textContent=oledAvailable()?'OLED respondeu. Controles liberados.':'OLED nao respondeu; o restante do JrBot continua disponivel.';const micInfo=el('test_mic')?.nextElementSibling;if(micInfo)micInfo.textContent=next.mic==='available'?'MS3625 detectou atividade I2S. Teste liberado.':'MS3625 sem atividade I2S. Conecte e clique Atualizar estado.';connection(valid?(mode==='serial'?'Painel conectado em '+(activePort||'Serial')+' - firmware confirmado':'Wi-Fi - firmware confirmado'):'Firmware nao confirmado',valid);refreshControls();}
async function api(path,opts={}){const headers={...(opts.headers||{})};if(opts.method==='POST')headers['X-JrBot-Panel']='1';const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),35000);try{const r=await fetch(path,{...opts,headers,signal:controller.signal});const text=await r.text();if(!r.ok)throw new Error(text||('HTTP '+r.status));return text;}catch(e){if(e.name==='AbortError')throw new Error('Tempo de resposta esgotado.');throw e;}finally{clearTimeout(timer);}}
async function action(work,messageId='device_msg'){if(busy)return;busy=true;refreshControls();try{return await work();}catch(e){el(messageId).textContent='Erro: '+e.message;localLine('ERRO: '+e.message);}finally{busy=false;refreshControls();}}
async function setMode(m){if(busy)return;mode=m==='wifi'?'wifi':'serial';document.querySelector('input[value='+mode+']').checked=true;el('lbl_serial').classList.toggle('active',mode==='serial');el('lbl_wifi').classList.toggle('active',mode==='wifi');el('serialbar').classList.toggle('show',mode==='serial');el('wifibar').classList.toggle('show',mode==='wifi');invalidate('Selecione a conexao e clique em Conectar ou Atualizar estado.');connection('Conexao nao verificada',false);if(mode==='wifi'){await api('/disconnect',{method:'POST'});serialConnected=false;activePort='';localStorage.setItem('jr_esp_ip',val('esp_ip'));}}
async function refreshPorts(){try{const j=JSON.parse(await api('/ports'));const available=[...new Set((j.ports||[]).filter(p=>/^COM\d+$/i.test(p)).map(p=>p.toUpperCase()))].sort((a,b)=>Number(a.slice(3))-Number(b.slice(3)));const previous=(val('port')||localStorage.getItem('jr_serial_port')||'').toUpperCase();el('port').textContent='';if(!available.length){const o=document.createElement('option');o.value='';o.textContent='Nenhuma porta COM detectada';el('port').appendChild(o);return;}for(const p of available){const o=document.createElement('option');o.value=p;o.textContent=p;el('port').appendChild(o);}el('port').value=available.includes(previous)?previous:available[0];}catch(e){localLine('ERRO: '+e.message);}}
async function connect(){return action(async()=>{const port=val('port').toUpperCase();if(!/^COM\d+$/.test(port))throw new Error('Selecione uma porta COM detectada e clique Atualizar portas.');invalidate('Abrindo '+port+' e consultando o firmware...');const t=await api('/connect',{method:'POST',body:new URLSearchParams({port})});serialConnected=true;activePort=port;localStorage.setItem('jr_serial_port',port);connection('Porta aberta; consultando firmware',false);localLine(t);await new Promise(resolve=>setTimeout(resolve,1200));await send('status');});}
async function disconnect(){return action(async()=>{await api('/disconnect',{method:'POST'});serialConnected=false;activePort='';invalidate('Desconectado.');connection('Desconectado',false);});}
async function send(command){const verb=command.trim().toLowerCase().split(/\s+/)[0];if(['audio_test','som','beep','mic_test'].includes(verb)&&!currentFirmware())throw new Error('Teste bloqueado: grave uma versao JrBot_V1.x / HW04.');if(new TextEncoder().encode(command).length>768)throw new Error('Comando excede 768 bytes.');let t;try{t=await api('/send',{method:'POST',body:new URLSearchParams({command,mode,ip:val('esp_ip')})});}catch(e){if(command.trim().toLowerCase()==='status'){invalidate('Firmware nao confirmado.');connection('Firmware sem confirmacao',false);}throw e;}if(t.trim())localLine(t.trim());if(command.trim().toLowerCase()==='status')renderStatus(t);if(mode==='wifi')localStorage.setItem('jr_esp_ip',val('esp_ip'));return t;}
async function refreshStatus(){return action(()=>send('status'));}
async function checkVersion(){return action(async()=>{await send('status');el('device_msg').textContent='Firmware confirmado: '+device.version+' | '+device.hardware+' | '+device.profile;});}
function readRuntimeApiReply(reply){
  const prefix='JR_API ';
  if(!reply.startsWith(prefix))throw new Error('Resposta nao pertence a Runtime API: '+reply);
  let body;
  try{body=JSON.parse(reply.slice(prefix.length));}catch(_){throw new Error('Runtime API devolveu JSON invalido.');}
  if(body.ok!==true)throw new Error('Runtime API recusou a chamada: '+JSON.stringify(body.error||{}));
  return body;
}
function parseJrSkillJson(raw,label){
  let doc;
  try{doc=JSON.parse(raw);}catch(_){throw new Error(label+' contem JSON invalido.');}
  if(!doc||typeof doc!=='object'||Array.isArray(doc))throw new Error(label+' deve ser um objeto JSON.');
  if(doc.v!==1)throw new Error(label+' requer v=1.');
  if(!Array.isArray(doc.run)||!doc.run.length)throw new Error(label+' precisa de run[].');
  if(doc.run.length>64)throw new Error(label+' excede 64 chamadas.');
  return doc;
}
function validateCall(call,label){
  if(!Array.isArray(call)||!call.length||typeof call[0]!=='string')throw new Error(label+' possui chamada invalida.');
  if(!/^[a-z][a-z0-9_.-]{0,63}$/.test(call[0]))throw new Error(label+' possui nome de funcao invalido.');
}
async function runtimeCapabilities(sender=send){
  const reply=await sender('api '+JSON.stringify({v:1,fn:'capabilities',args:{}}));
  const body=readRuntimeApiReply(reply);
  const actions=body.result?.actions;
  if(!Array.isArray(actions))throw new Error('Runtime API nao informou actions.');
  return new Set(actions);
}
async function executeJrSkillDocument(doc,ctx,label){
  if(ctx.depth>4)throw new Error('Limite de profundidade de recipes excedido.');
  for(let i=0;i<doc.run.length;i++){
    ctx.guard?.();
    const call=doc.run[i];
    validateCall(call,label);
    const fn=call[0];
    const position=(i+1)+'/'+doc.run.length;
    if(fn==='wait'){
      if(call.length!==2||!Number.isInteger(call[1])||call[1]<0||call[1]>5000)throw new Error(label+' wait invalido.');
      localLine('JR_SKILL_V1 call='+position+' fn=wait ms='+call[1]+' depth='+ctx.depth);
      await new Promise(resolve=>setTimeout(resolve,call[1]));
      ctx.guard?.();
      continue;
    }
    if(fn==='recipe'){
      if(call.length!==2||typeof call[1]!=='string'||!/^[A-Za-z0-9_-]{1,64}$/.test(call[1]))throw new Error(label+' recipe invalida.');
      const name=call[1];
      if(ctx.stack.includes(name))throw new Error('Loop de recipe detectado: '+ctx.stack.concat([name]).join(' -> '));
      localLine('JR_SKILL_V1 recipe_enter='+name+' depth='+(ctx.depth+1));
      const raw=await api('/jrskill/recipe/'+encodeURIComponent(name));
      const recipe=parseJrSkillJson(raw,'recipe '+name);
      await executeJrSkillDocument(recipe,{...ctx,depth:ctx.depth+1,stack:ctx.stack.concat([name])},'recipe '+name);
      localLine('JR_SKILL_V1 recipe_exit='+name+' depth='+(ctx.depth+1));
      continue;
    }
    if(fn==='face'){
      if(call.length!==2||typeof call[1]!=='string'||!/^[a-z_]{1,32}$/.test(call[1]))throw new Error(label+' face invalida.');
      if(!ctx.actions.has('face'))throw new Error('Capability face nao disponivel neste firmware.');
      localLine('JR_SKILL_V1 call='+position+' fn=face value='+call[1]+' depth='+ctx.depth);
      ctx.guard?.();
      const reply=await (ctx.sender||send)('api '+JSON.stringify({v:1,fn:'face',args:{expression:call[1]}}));
      ctx.guard?.();
      const body=readRuntimeApiReply(reply);
      if(body.result?.expression!==call[1])throw new Error('Runtime API nao confirmou face '+call[1]+'.');
      continue;
    }
    throw new Error('Funcao JrSkill nao permitida nesta prova: '+fn);
  }
}
function renderDevnetState(state){
  devnetState=state;
  const badge=el('jrskill_devnet_status');
  badge.textContent='Solana Devnet: '+(state==='available'?'disponivel':state==='unavailable'?'indisponivel':'verificando...');
  badge.className='pill '+(state==='available'?'ok':state==='unavailable'?'bad':'');
  el('jrskill_devnet_msg').textContent=state==='available'?'Consulta Devnet disponivel. Autentique a carteira licenciada e conecte o robo pela Serial para executar.':state==='unavailable'?'Sem acesso a Devnet. A Skill local e os controles pela Serial continuam disponiveis. Reconecte a Internet e clique Verificar Devnet.':'Verificando acesso a Solana. A Skill local funciona sem Internet.';
  refreshControls();
}
async function checkDevnetConnection(){
  if(devnetChecking||busy)return;
  devnetChecking=true;
  renderDevnetState('unknown');
  try{
    const status=JSON.parse(await api('/jrskill/devnet-status'));
    renderDevnetState(status.available===true&&status.cluster==='devnet'?'available':'unavailable');
  }catch(_){renderDevnetState('unavailable');}
  finally{devnetChecking=false;refreshControls();}
}
async function monitorDevnet(){
  try{await checkDevnetConnection();}finally{setTimeout(monitorDevnet,30000);}
}
async function runJrSkillTest(source='local-file'){
  return action(async()=>{
    let sender=send,guard=()=>{};
    const wallet=skillWallet;
    if(source==='solana-devnet')guard=()=>{if(!wallet.authenticated||!wallet.address||skillWallet.revision!==wallet.revision)throw new Error('Carteira desconectada, alterada ou nao autenticada; execucao Devnet bloqueada.');};
    try{
      guard();
      if(mode!=='serial')throw new Error('O teste JrSkill v1 usa somente a conexao Serial.');
      if(!serialConnected)throw new Error('Conecte a Serial antes de executar a Skill.');
      if(!currentFirmware())await send('status');
      if(!currentFirmware()||!oledAvailable())throw new Error('Firmware HW04/OLED nao confirmado.');
      el('jrskill_api_state').textContent='carregando';
      let raw;
      if(source==='solana-devnet'){
        if(devnetState!=='available')throw new Error('Devnet indisponivel ou ainda nao verificada. Clique Verificar Devnet.');
        const selected=el('jrskill_selected').value;
        if(!selected)throw new Error('Selecione uma Skill licenciada.');
        localLine('JR_SKILL_SOLANA fetch=started skill='+selected);
        let proof;
        try{
          proof=JSON.parse(await api('/jrskill/wallet/execution/start',{method:'POST',body:new URLSearchParams({skill:selected})}));
          guard();
          if(proof.pda!==selected||proof.buyer!==wallet.address||typeof proof.execution_permit!=='string'||!proof.execution_permit||!proof.license)throw new Error('Autorizacao nao corresponde a carteira.');
          sender=async command=>{guard();const result=JSON.parse(await api('/jrskill/wallet/execution/send',{method:'POST',body:new URLSearchParams({permit:proof.execution_permit,command})}));guard();if(typeof result.reply!=='string')throw new Error('Resposta autorizada invalida');localLine(result.reply);return result.reply;};
          localLine('JR_SKILL_AUTHORIZATION result=allowed buyer='+proof.buyer+' license='+proof.license);
          if(proof.source!==source||proof.hash_verified!==true||proof.commitment!=='finalized'||typeof proof.payload_text!=='string')throw new Error('Prova Devnet invalida.');
        }catch(e){renderDevnetState('unavailable');throw e;}
        raw=proof.payload_text;
        localLine('JR_SKILL_SOLANA source='+source+' pda='+proof.pda+' rpc_slot='+proof.rpc_slot+' payload_bytes='+proof.payload_bytes+' payload_hash='+proof.payload_hash+' hash_verified=true matches_checkpoint='+proof.matches_checkpoint);
      }else if(source==='local-file'){
        raw=await api('/jrskill/skill');
      }else throw new Error('Origem da Skill invalida.');
      const skill=parseJrSkillJson(raw,'skill');
      const actions=await runtimeCapabilities(sender);
      el('jrskill_api_state').textContent='executando';
      el('jrskill_api_msg').textContent='Executando JSON minimo v1 com '+skill.run.length+' chamadas de nivel principal...';
      localLine('JR_SKILL_V1 start=v1 source='+source+' top_calls='+skill.run.length);
      await executeJrSkillDocument(skill,{actions,depth:0,stack:[],sender,guard},'skill');
      guard();
      el('jrskill_api_state').textContent='ok';
      el('jrskill_api_msg').textContent='Skill concluida ('+source+'): result=ok. Confirme a sequencia visual no robo.';
      localLine('JR_SKILL_V1 result=ok source='+source);
    }catch(e){
      if(source==='solana-devnet')localLine('JR_SKILL_AUTHORIZATION result=blocked detail='+e.message);
      el('jrskill_api_state').textContent='erro';
      localLine('JR_SKILL_V1 result=error source='+source+' detail='+e.message);
      throw e;
    }
  },'jrskill_api_msg');
}
async function sendCustom(){const c=el('custom').value;if(c.trim())return action(()=>send(c));}
async function connectWifi(){if(busy)return;await setMode('wifi');return refreshStatus();}
async function testWifi(){return connectWifi();}
async function setAudioVolume(){return action(async()=>{const v=val('audio_volume');el('audioVolText').textContent=v+'%';const t=await send('audio_volume '+v);if(fields(t).audio_volume!==v)throw new Error('Volume nao confirmado.');if(device)device.volume=v;el('audio_msg').textContent='Volume confirmado: '+v+'%. Agora clique Testar audio.';},'audio_msg');}
async function testAudio(){return action(async()=>{if(!currentFirmware()||!['ready','on_demand'].includes(device.audio))throw new Error('Audio nao disponivel.');el('audio_msg').textContent='Executando um unico teste I2S com o volume ja aplicado...';const reply=await send('audio_test'),f=fields(reply);if(f.audio_test!=='tx_completed')throw new Error('Teste de audio nao confirmado.');el('audio_msg').textContent='I2S confirmado: teste #'+(f.seq||'?')+', '+(f.bytes||'?')+' bytes, '+(f.freq||'?')+' Hz. Se nao ouviu, o proximo foco e MAX98357A/falante/alimentacao.';},'audio_msg');}
async function testMic(){return action(async()=>{if(!currentFirmware()||device?.mic!=='available')throw new Error('Microfone nao detectado. Clique Atualizar estado.');el('mic_state').textContent='Testando MS3625...';const reply=await send('mic_test'),f=fields(reply);if(f.mic_test!=='signal_detected')throw new Error('Sinal do microfone nao confirmado.');el('mic_state').textContent='OK - MS3625 canal '+f.channel+' | amostras '+f.samples+' | pico '+f.peak_raw+' | mudancas '+f.changes;},'device_msg');}
async function testCamera(){return action(async()=>{if(mode!=='serial')throw new Error('Use Serial USB na porta COM selecionada para testar a camera.');if(!device||device.camera!=='available')throw new Error('Camera nao detectada. Clique Atualizar estado.');el('cam_msg').textContent='Capturando um quadro da OV5640...';const t=await send('camera_test'),f=fields(t);if(f.camera_test!=='frame_received'||f.released!=='1')throw new Error('Captura nao confirmada.');el('cam_msg').textContent='OK - quadro '+f.width+' x '+f.height+', '+f.bytes+' bytes, sensor '+f.pid+'.';},'cam_msg');}
function takePhoto(){el('cam_msg').textContent='Use Testar camera para verificar a captura.';}function openCameraPortal(){takePhoto();}
function saveWifiLocal(){for(const id of ['wifi_ssid','wifi_host','wifi_static','wifi_ip','wifi_gw','wifi_mask','wifi_dns1','wifi_dns2','esp_ip'])localStorage.setItem('jr_'+id,el(id).value);}
function loadWifiLocal(){for(const id of ['wifi_ssid','wifi_host','wifi_static','wifi_ip','wifi_gw','wifi_mask','wifi_dns1','wifi_dns2','esp_ip']){const v=localStorage.getItem('jr_'+id);if(v!==null)el(id).value=v;}}
function encodedField(id,max){const text=el(id).value;if(/[\x00-\x1f\x7f]/.test(text))throw new Error('Caracteres de controle nao permitidos.');if(new TextEncoder().encode(text).length>max)throw new Error(id+' excede '+max+' bytes.');return encodeURIComponent(text);}
async function configureWifi(){return action(async()=>{if(mode!=='serial')throw new Error('Use Serial USB antes de salvar Wi-Fi.');const ssid=encodedField('wifi_ssid',32);if(!ssid)throw new Error('Informe o SSID.');const pairs=[['ssid',ssid],['pass',encodedField('wifi_pass',64)],['host',encodedField('wifi_host',32)],['static',val('wifi_static')],['ip',encodedField('wifi_ip',15)],['gw',encodedField('wifi_gw',15)],['mask',encodedField('wifi_mask',15)],['dns1',encodedField('wifi_dns1',15)],['dns2',encodedField('wifi_dns2',15)]];const reply=await send('wifi_config_pct '+pairs.map(([k,v])=>k+'='+v).join('|'));if(!reply.startsWith('JR_WIFI_SALVO'))throw new Error('Salvamento nao confirmado.');saveWifiLocal();el('wifi_pass').value='';el('device_msg').textContent='Wi-Fi salvo. Reinicie a placa e clique Atualizar estado.';});}
async function clearWifi(){if(confirm('Limpar configuracao Wi-Fi?'))return action(async()=>{if(mode!=='serial')throw new Error('Use Serial USB.');await send('wifi_clear');el('device_msg').textContent='Configuracao Wi-Fi limpa. Reinicie a placa.';});}
async function poll(){try{const j=JSON.parse(await api('/logs?after='+serverCursor+'&session='+encodeURIComponent(serverSession)));if(serverSession&&serverSession!==j.session){serverCursor=0;invalidate('Servidor reiniciado. Reconecte.');}serverSession=j.session;if(j.gap)localLine('Aviso: registros antigos expiraram.');appendLog(j.logs);serverCursor=j.next_cursor;if(mode==='serial'&&!busy){serialConnected=j.serial_connected;activePort=j.serial_port||'';if(!serialConnected){if(device)invalidate('Serial desconectada.');connection('Serial desconectado',false);}else if(!device)connection('Porta aberta; firmware nao confirmado',false);}}catch(e){if(!busy){invalidate('Servidor do painel indisponivel.');connection('Servidor indisponivel',false);}}finally{setTimeout(poll,600);}}
for(const [name,command] of faces){const b=document.createElement('button');b.className='face';b.textContent=name;b.onclick=()=>action(()=>send(command));el('faces').appendChild(b);}
const micButton=el('test_mic');if(micButton){micButton.textContent='Testar microfone';micButton.className='green';micButton.onclick=()=>testMic();if(micButton.nextElementSibling)micButton.nextElementSibling.textContent='MS3625: o painel libera o teste quando detectar atividade I2S.';}
el('custom').addEventListener('keydown',e=>{if(e.key==='Enter')sendCustom();});logEl.addEventListener('scroll',()=>{autoScroll=logEl.scrollTop+logEl.clientHeight>=logEl.scrollHeight-20;});window.addEventListener('unhandledrejection',event=>{event.preventDefault();localLine('ERRO: '+String(event.reason?.message||event.reason));});
const sub=document.querySelector('header .sub');if(sub)sub.textContent='JrBot V1 | portas COM detectadas automaticamente';
loadWifiLocal();setMode('serial').catch(e=>localLine('ERRO: '+e.message));refreshPorts();poll();refreshControls();
monitorDevnet();
