'use strict';
const faces=[['Neutro','neutro'],['Feliz','feliz'],['Triste','triste'],['Animado','animado'],['Bravo','bravo'],['Surpreso','surpreso'],['Pensando','pensando'],['Cetico','cetico'],['Sono','sono'],['Confuso','confuso'],['Piscando','piscando'],['Amor','amor'],['Brincalhao','brincalhao'],['Preocupado','preocupado'],['Cool','cool'],['Bateria','bateria']];
const el=id=>document.getElementById(id), val=id=>el(id).value.trim();
const logEl=el('log');
let serverCursor=0, serverSession='', autoScroll=true, busy=false, device=null;
// Start in Serial intentionally; never silently send to a saved Wi-Fi address.
let mode='serial', serialConnected=false, activePort='';
function appendLog(items){
  for(const item of items){const line=document.createElement('div');line.textContent='['+item.ts+'] '+item.line;logEl.appendChild(line);el('last').textContent=item.ts;}
  while(logEl.children.length>1200)logEl.removeChild(logEl.firstChild);
  if(autoScroll)logEl.scrollTop=logEl.scrollHeight;
}
function localLine(line){appendLog([{ts:new Date().toLocaleTimeString(),line}]);}
function clearLog(){logEl.textContent='';}
function downloadLog(){const blob=new Blob([Array.from(logEl.children,n=>n.textContent).join('\n')],{type:'text/plain;charset=utf-8'});const url=URL.createObjectURL(blob),a=document.createElement('a');a.href=url;a.download='JrBot-painel-log.txt';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}
function openWifiConfig(){el('wifi_config_title').scrollIntoView({behavior:'smooth'});}
function connection(text,ok){el('conn').textContent=text;el('conn').className='pill '+(ok?'ok':'bad');}
function fields(text){const result={};for(const token of text.trim().split(/\s+/)){const at=token.indexOf('=');if(at>0)result[token.slice(0,at)]=token.slice(at+1);}return result;}
function refreshControls(){
  document.querySelectorAll('[data-action]').forEach(b=>b.disabled=busy);
  const sound=!!device&&['ready','on_demand'].includes(device.audio);
  const camera=!!device&&mode==='serial'&&device.camera==='on_demand';
  el('test_audio').disabled=busy||!sound;
  el('test_camera').disabled=busy||!camera;
  el('test_mic').disabled=true; // No microphone driver or wiring supplied.
  document.querySelectorAll('#faces button,#demo').forEach(b=>b.disabled=busy||!device||device.oled==='disabled');
  el('audio_volume').disabled=busy||!device;
  el('apply_volume').disabled=busy||!device;
}
function invalidate(message){device=null;el('device_msg').textContent=message;for(const id of ['fw_version','fw_profile','oled_state','audio_state','camera_state','mic_state'])el(id).textContent='Nao verificado';refreshControls();}
function renderStatus(text){
  if(!text.startsWith('JR_STATUS protocol=2 ')){invalidate('Firmware sem protocolo V2.');connection('Firmware nao confirmado',false);throw new Error('Firmware sem protocolo V2. Atualize o firmware; abrir a COM nao confirma a versao.');}
  const next=fields(text);
  if(!next.version||!next.profile||!next.oled){invalidate('Estado incompleto.');connection('Firmware nao confirmado',false);throw new Error('Estado incompleto do firmware; diagnostico nao confirmado.');}
  device=next;
  el('face_controls').open=next.oled!=='disabled';
  el('fw_version').textContent=next.version;
  el('fw_profile').textContent=next.profile;
  el('oled_state').textContent=next.oled==='disabled'?'Desabilitado - nao bloqueia o robo':next.oled;
  el('audio_state').textContent=next.audio==='disabled'?'Desabilitado no firmware':next.audio==='on_demand'?'Disponivel para teste':next.audio||'Nao informado';
  el('camera_state').textContent=next.camera==='disabled'?'Desabilitada no firmware':next.camera==='on_demand'?'Teste disponivel por Serial':next.camera||'Nao informado';
  el('mic_state').textContent='Nao configurado - informe modelo e ligacao';
  el('device_msg').textContent='Firmware respondeu. Estado consultado em '+new Date().toLocaleTimeString()+'.'+(next.pending_restart==='1'?' Configuracao de rede pendente de reinicializacao.':'');
  el('audio_msg').textContent=next.audio==='disabled'?'O firmware ainda bloqueia o audio. A habilitacao depende de confirmar MAX98357A e GPIO39/40/41; este painel nao altera essa protecao.':'Teste curto e sob demanda. Envio I2S concluido nao comprova som audivel.';
  el('cam_msg').textContent=next.camera==='disabled'?'Camera bloqueada no firmware ate confirmar o mapa de pinos. Nao e erro do OLED.':mode==='wifi'?'Nesta versao, use Serial USB / COM4 no proprio painel para testar a camera.':'O teste informa sensor, tamanho e bytes do quadro. Previa de foto e autofocus ainda indisponiveis.';
  el('face_msg').textContent=next.oled==='disabled'?'OLED desabilitado. Os outros testes continuam independentes.':'Rostos enviam comandos; envio aceito nao comprova imagem visivel.';
  connection(mode==='serial'?'Painel conectado em '+(activePort||'Serial')+' - V2 confirmada':'Wi-Fi - V2 confirmada',true);
  refreshControls();
}
async function api(path,opts={}){
  const headers={...(opts.headers||{})};if(opts.method==='POST')headers['X-JrBot-Panel']='1';
  const controller=new AbortController(), timer=setTimeout(()=>controller.abort(),35000);
  try{const r=await fetch(path,{...opts,headers,signal:controller.signal});const text=await r.text();if(!r.ok)throw new Error(text||('HTTP '+r.status));return text;}
  catch(e){if(e.name==='AbortError')throw new Error('Tempo de resposta esgotado. Execucao nao confirmada; nao reenviada automaticamente.');throw e;}
  finally{clearTimeout(timer);}
}
async function action(work,messageId='device_msg'){
  if(busy)return;busy=true;refreshControls();
  try{return await work();}catch(e){el(messageId).textContent='Erro: '+e.message;localLine('ERRO: '+e.message);}
  finally{busy=false;refreshControls();}
}
async function setMode(m){
  if(busy)return;
  mode=m==='wifi'?'wifi':'serial';
  document.querySelector('input[value='+mode+']').checked=true;
  el('lbl_serial').classList.toggle('active',mode==='serial');el('lbl_wifi').classList.toggle('active',mode==='wifi');
  el('serialbar').classList.toggle('show',mode==='serial');el('wifibar').classList.toggle('show',mode==='wifi');
  invalidate('Selecione a conexao e clique em Conectar ou Atualizar estado.');
  connection('Conexao nao verificada',false);
  if(mode==='wifi'){await api('/disconnect',{method:'POST'});serialConnected=false;activePort='';localStorage.setItem('jr_esp_ip',val('esp_ip'));}
}
async function refreshPorts(){try{
  const j=JSON.parse(await api('/ports'));const previous=val('port')||localStorage.getItem('jr_serial_port')||'COM4';
  el('port').textContent='';const empty=document.createElement('option');empty.value='';empty.textContent='Selecione a porta de comandos';el('port').appendChild(empty);
  for(const p of j.ports){const o=document.createElement('option');o.value=p;o.textContent=p+(p==='COM4'?' - painel (sua montagem)':p==='COM6'?' - gravacao (sua montagem)':'');el('port').appendChild(o);}
  el('port').value=j.ports.includes(previous)?previous:j.ports.includes('COM4')?'COM4':'';
}catch(e){localLine('ERRO: '+e.message);}}
async function connect(){return action(async()=>{
  const port=val('port');if(!port)throw new Error('Selecione COM4, ou a porta de comandos renumerada pelo Windows.');
  if(port==='COM6'&&!confirm('Voce informou COM6 para gravacao e COM4 para comandos. Abrir COM6 mesmo assim?'))return;
  invalidate('Abrindo a porta e consultando o firmware...');
  const t=await api('/connect',{method:'POST',body:new URLSearchParams({port})});serialConnected=true;activePort=port;localStorage.setItem('jr_serial_port',port);
  connection('Porta aberta; firmware ainda nao confirmado',false);localLine(t);
  await new Promise(resolve=>setTimeout(resolve,1200));
  await send('status');
});}
async function disconnect(){return action(async()=>{await api('/disconnect',{method:'POST'});serialConnected=false;activePort='';invalidate('Desconectado.');connection('Desconectado',false);});}
async function send(command){
  if(new TextEncoder().encode(command).length>768)throw new Error('Comando excede 768 bytes; nada enviado.');
  let t;
  try{t=await api('/send',{method:'POST',body:new URLSearchParams({command,mode,ip:val('esp_ip')})});}
  catch(e){if(command.trim().toLowerCase()==='status'){invalidate('Firmware nao confirmado. Confira a versao gravada e a conexao.');connection('Firmware sem confirmacao',false);}throw e;}
  // Do not echo submitted commands: they can contain Wi-Fi passwords.
  if(t.trim())localLine(t.trim());
  if(command.trim().toLowerCase()==='status')renderStatus(t);
  if(mode==='wifi')localStorage.setItem('jr_esp_ip',val('esp_ip'));
  return t;
}
async function refreshStatus(){return action(()=>send('status'));}
async function checkVersion(){return action(async()=>{await send('status');el('device_msg').textContent='Versao confirmada: '+device.version+' | perfil: '+device.profile;});}
async function sendCustom(){const c=el('custom').value;if(c.trim())return action(()=>send(c));}
async function connectWifi(){if(busy)return;await setMode('wifi');return refreshStatus();}
async function testWifi(){return connectWifi();}
async function setAudioVolume(){return action(async()=>{
  const v=val('audio_volume');el('audioVolText').textContent=v+'%';el('audio_msg').textContent='Aguardando confirmacao do volume...';
  const t=await send('audio_volume '+v);
  if(fields(t).audio_volume!==v)throw new Error('Volume devolvido difere do solicitado.');
  el('audio_msg').textContent='Volume confirmado: '+v+'%.'+(t.includes('audio=disabled')?' Saida desabilitada no firmware.':'');
},'audio_msg');}
async function testAudio(){return action(async()=>{
  if(!device||!['ready','on_demand'].includes(device.audio))throw new Error('Audio nao habilitado no firmware.');
  el('audio_msg').textContent='Executando teste; aguarde a resposta...';
  const v=val('audio_volume'), volumeReply=await send('audio_volume '+v);
  if(fields(volumeReply).audio_volume!==v)throw new Error('Volume nao confirmado; teste nao iniciado.');
  const reply=await send('audio_test');
  if(!/^JR_OK audio_test=(tx_completed|completed)( |$)/.test(reply))throw new Error('Conclusao do teste nao confirmada.');
  el('audio_msg').textContent='Envio do tom concluido. Confirme se ouviu o som: a resposta nao comprova o falante.';
},'audio_msg');}
async function testCamera(){return action(async()=>{
  if(mode!=='serial')throw new Error('Use Serial USB / COM4 neste painel para testar a camera.');
  if(!device||device.camera!=='on_demand')throw new Error('Camera nao habilitada no firmware.');
  el('cam_msg').textContent='Capturando um quadro. Aguarde; nao e uma transmissao de video...';
  const t=await send('camera_test'), f=fields(t);
  if(f.camera_test!=='frame_received'||f.released!=='1')throw new Error('Captura e liberacao nao confirmadas.');
  el('cam_msg').textContent='Quadro recebido: '+f.width+' x '+f.height+', '+f.bytes+' bytes, sensor '+f.pid+'. Recursos liberados. Imagem e foco ainda nao avaliados.';
},'cam_msg');}
function takePhoto(){el('cam_msg').textContent='Previa de foto indisponivel nesta versao. Use Testar camera para verificar uma captura.';}
function openCameraPortal(){takePhoto();}
function saveWifiLocal(){for(const id of ['wifi_ssid','wifi_host','wifi_static','wifi_ip','wifi_gw','wifi_mask','wifi_dns1','wifi_dns2','esp_ip'])localStorage.setItem('jr_'+id,el(id).value);}
function loadWifiLocal(){for(const id of ['wifi_ssid','wifi_host','wifi_static','wifi_ip','wifi_gw','wifi_mask','wifi_dns1','wifi_dns2','esp_ip']){const v=localStorage.getItem('jr_'+id);if(v!==null)el(id).value=v;}}
function encodedField(id,max){const text=el(id).value;if(/[\x00-\x1f\x7f]/.test(text))throw new Error('Caracteres de controle nao permitidos.');if(new TextEncoder().encode(text).length>max)throw new Error(id+' excede '+max+' bytes.');return encodeURIComponent(text);}
async function configureWifi(){return action(async()=>{
  if(mode!=='serial')throw new Error('Selecione Serial USB e conecte a COM4 antes de salvar Wi-Fi.');
  const ssid=encodedField('wifi_ssid',32);if(!ssid)throw new Error('Informe o SSID.');
  const pairs=[['ssid',ssid],['pass',encodedField('wifi_pass',64)],['host',encodedField('wifi_host',32)],['static',val('wifi_static')],['ip',encodedField('wifi_ip',15)],['gw',encodedField('wifi_gw',15)],['mask',encodedField('wifi_mask',15)],['dns1',encodedField('wifi_dns1',15)],['dns2',encodedField('wifi_dns2',15)]];
  const reply=await send('wifi_config_pct '+pairs.map(([k,v])=>k+'='+v).join('|'));
  if(!reply.startsWith('JR_WIFI_SALVO'))throw new Error('Salvamento nao confirmado.');
  saveWifiLocal();el('wifi_pass').value='';el('device_msg').textContent='Wi-Fi salvo. Reinicie a placa e clique em Atualizar estado para conferir o IP.';
});}
async function clearWifi(){if(confirm('Limpar configuracao Wi-Fi? Exige reiniciar.'))return action(async()=>{if(mode!=='serial')throw new Error('Use a conexao Serial no painel.');await send('wifi_clear');el('device_msg').textContent='Configuracao limpa. Reinicie a placa.';});}
async function poll(){try{
  const j=JSON.parse(await api('/logs?after='+serverCursor+'&session='+encodeURIComponent(serverSession)));
  if(serverSession&&serverSession!==j.session){serverCursor=0;invalidate('Servidor reiniciado. Reconecte e atualize o estado.');localLine('Servidor do painel reiniciado.');}
  serverSession=j.session;if(j.gap)localLine('Aviso: registros antigos expiraram.');appendLog(j.logs);serverCursor=j.next_cursor;
  if(mode==='serial'&&!busy){
    serialConnected=j.serial_connected;activePort=j.serial_port||'';
    if(!serialConnected){if(device)invalidate('Serial desconectada.');connection('Serial desconectado',false);}
    else if(!device)connection('Porta aberta; firmware nao confirmado',false);
  }
}catch(e){if(!busy){invalidate('Servidor do painel indisponivel. Reabra PAINEL.bat.');connection('Servidor indisponivel',false);}}
finally{setTimeout(poll,600);}}
for(const [name,command] of faces){const b=document.createElement('button');b.className='face';b.textContent=name;b.onclick=()=>action(()=>send(command));el('faces').appendChild(b);}
el('custom').addEventListener('keydown',e=>{if(e.key==='Enter')sendCustom();});
logEl.addEventListener('scroll',()=>{autoScroll=logEl.scrollTop+logEl.clientHeight>=logEl.scrollHeight-20;});
window.addEventListener('unhandledrejection',event=>{event.preventDefault();localLine('ERRO: '+String(event.reason?.message||event.reason));});
loadWifiLocal(); // Volume starts at 10%; do not restore a previous loud setting.
setMode('serial').catch(e=>localLine('ERRO: '+e.message));refreshPorts();poll();refreshControls();
