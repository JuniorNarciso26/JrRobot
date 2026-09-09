/* Camera + audio/microfone V2: funcoes unificadas no painel local. */
let jrPhotoUrl='';
let jrMicUrl='';

function jrValidIpv4(text){
  return /^\d{1,3}(?:\.\d{1,3}){3}$/.test(String(text||''));
}

function jrNormalizeCameraUi(){
  const box=document.getElementById('camera_box');
  if(!box) return;
  for(const button of Array.from(box.querySelectorAll('button'))){
    if(button.id==='test_camera') continue;
    const text=(button.textContent||'').trim();
    if(/previa|ver foto|testar camera pelo wi-fi/i.test(text)) button.remove();
  }
  const testButton=document.getElementById('test_camera');
  if(testButton) testButton.textContent='Testar camera';
}

function jrNormalizeAudioMicUi(){
  const audioBox=document.getElementById('audio_box');
  if(audioBox && !document.getElementById('play_recording')){
    const play=document.createElement('button');
    play.id='play_recording';
    play.className='purple';
    play.dataset.action='1';
    play.textContent='Tocar gravacao na caixinha';
    play.onclick=()=>playRecordingOnSpeaker();
    const msg=document.getElementById('audio_msg');
    audioBox.insertBefore(play,msg||null);
  }

  let micBox=document.getElementById('mic_box');
  if(!micBox){
    for(const heading of Array.from(document.querySelectorAll('h2'))){
      if((heading.textContent||'').trim().toLowerCase()==='microfone'){
        micBox=heading.nextElementSibling;
        if(micBox) micBox.id='mic_box';
        break;
      }
    }
  }
  if(!micBox) return;

  const test=document.getElementById('test_mic');
  if(test){
    test.textContent='Testar microfone';
    test.className='green';
    test.onclick=()=>testMic();
    test.dataset.action='1';
  }
  if(!document.getElementById('record_mic')){
    const record=document.createElement('button');
    record.id='record_mic';
    record.className='purple';
    record.dataset.action='1';
    record.textContent='Gravar 3s';
    record.onclick=()=>recordMic();
    micBox.appendChild(record);
  }
  let msg=document.getElementById('mic_msg');
  if(!msg){
    msg=document.createElement('div');
    msg.id='mic_msg';
    msg.className='msg';
    micBox.appendChild(msg);
  }
  if(!document.getElementById('mic_audio')){
    const audio=document.createElement('audio');
    audio.id='mic_audio';
    audio.controls=true;
    audio.style.width='100%';
    audio.style.display='none';
    micBox.appendChild(audio);
  }
  if(!document.getElementById('mic_download')){
    const download=document.createElement('a');
    download.id='mic_download';
    download.className='pill';
    download.download='jrbot-microfone.wav';
    download.textContent='Baixar WAV';
    download.style.display='none';
    micBox.appendChild(download);
  }
}

const jrBaseRefreshControls=refreshControls;
refreshControls=function(){
  jrNormalizeCameraUi();
  jrNormalizeAudioMicUi();
  jrBaseRefreshControls();

  const cameraButton=document.getElementById('test_camera');
  if(cameraButton){
    const cameraReady=currentFirmware()&&device?.camera==='available';
    const wifiReady=cameraReady&&device?.wifi==='1'&&jrValidIpv4(val('esp_ip'));
    cameraButton.disabled=busy||(mode==='wifi'?!wifiReady:!cameraReady);
    cameraButton.textContent='Testar camera';
  }

  const valid=currentFirmware();
  const voiceBusy=device?.voice_running==='1';
  const micButton=document.getElementById('test_mic');
  if(micButton) micButton.disabled=busy||!valid||voiceBusy||['busy','error'].includes(device?.mic);
  const record=document.getElementById('record_mic');
  if(record) record.disabled=busy||!valid||voiceBusy||device?.wifi!=='1'||!jrValidIpv4(val('esp_ip'));
  const play=document.getElementById('play_recording');
  if(play) play.disabled=busy||!valid||voiceBusy||device?.mic_has_recording!=='1';
};

const jrBaseRenderStatus=renderStatus;
renderStatus=function(text){
  const result=jrBaseRenderStatus(text);
  if(device?.wifi==='1'&&jrValidIpv4(device?.ip)){
    el('esp_ip').value=device.ip;
    localStorage.setItem('jr_esp_ip',device.ip);
  }
  if(device?.camera==='available'){
    el('cam_msg').textContent=device?.wifi==='1'
      ? 'OV5640 detectada. O mesmo botao Testar camera funciona pela Serial ou pelo Wi-Fi, conforme a conexao selecionada.'
      : 'OV5640 detectada. Teste pela Serial; quando o Wi-Fi estiver conectado, o mesmo botao tambem testa e mostra a foto pelo Wi-Fi.';
  }
  const micMsg=document.getElementById('mic_msg');
  if(micMsg){
    const has=device?.mic_has_recording==='1';
    const recorded=has?' Ultima gravacao: '+(device.mic_record_seconds||'?')+'s, '+(device.mic_record_samples||'?')+' amostras, pico '+(device.mic_record_peak||'0')+'.':'';
    if(device?.voice_running==='1'||device?.mic==='busy'){
      micMsg.textContent='MS3625 em uso por '+(device?.mic_owner||'voz')+'. Pare o modo autonomo/Playground antes de testes manuais.'+recorded;
    }else if(device?.mic==='unknown'){
      micMsg.textContent='MS3625 ainda indefinido neste boot. Use Testar microfone ou Gravar 3s para confirmar sinal real.'+recorded;
    }else if(device?.mic==='available'){
      micMsg.textContent='MS3625 confirmado. Grave 3s pelo Wi-Fi e ouca no painel; depois use Tocar gravacao na caixinha.'+recorded;
    }else{
      micMsg.textContent='MS3625 com erro de I/O: '+(device?.mic_error||'desconhecido')+'.'+recorded;
    }
  }
  refreshControls();
  return result;
};

async function takePhoto(){
  return action(async()=>{
    if(!currentFirmware()) throw new Error('Firmware JrBot V2 nao confirmado.');
    if(device?.camera!=='available') throw new Error('Camera nao detectada. Clique Atualizar estado.');
    if(device?.wifi!=='1') throw new Error('Wi-Fi da placa nao esta conectado.');
    const ip=val('esp_ip');
    if(!jrValidIpv4(ip)) throw new Error('Consulte o Wi-Fi da placa para obter o IP atual.');

    el('cam_msg').textContent='Testando a OV5640 pelo Wi-Fi em '+ip+'...';
    const controller=new AbortController();
    const timer=setTimeout(()=>controller.abort(),20000);
    let response;
    try{
      response=await fetch('/camera/capture?ip='+encodeURIComponent(ip)+'&t='+Date.now(),{cache:'no-store',signal:controller.signal});
    }catch(e){
      if(e.name==='AbortError') throw new Error('Tempo esgotado ao testar a camera pelo Wi-Fi.');
      throw e;
    }finally{
      clearTimeout(timer);
    }
    if(!response.ok){
      const detail=await response.text();
      throw new Error(detail||('Falha HTTP '+response.status));
    }
    const blob=await response.blob();
    if(blob.type!=='image/jpeg'||blob.size<4) throw new Error('A placa nao devolveu um JPEG valido.');

    if(jrPhotoUrl) URL.revokeObjectURL(jrPhotoUrl);
    jrPhotoUrl=URL.createObjectURL(blob);
    const img=el('cam_photo');
    await new Promise((resolve,reject)=>{
      img.onload=()=>resolve();
      img.onerror=()=>reject(new Error('O navegador nao conseguiu abrir o JPEG recebido.'));
      img.src=jrPhotoUrl;
    });
    img.style.display='block';
    el('cam_msg').textContent='Camera OK pelo Wi-Fi: '+img.naturalWidth+' x '+img.naturalHeight+', '+blob.size+' bytes, IP '+ip+'.';
    localLine('JR_CAMERA_FOTO ip='+ip+' width='+img.naturalWidth+' height='+img.naturalHeight+' bytes='+blob.size+' exibida=1');
  },'cam_msg');
}

async function recordMic(){
  return action(async()=>{
    if(!currentFirmware()) throw new Error('Firmware JrBot V2 nao confirmado.');
    if(device?.voice_running==='1') throw new Error('Microfone em uso pela voz. Pare autonomo/Playground antes de gravar.');
    if(device?.wifi!=='1') throw new Error('A gravacao WAV usa o Wi-Fi da placa. Consulte o Wi-Fi e confirme conexao.');
    const ip=val('esp_ip');
    if(!jrValidIpv4(ip)) throw new Error('Consulte o Wi-Fi da placa para obter o IP atual.');
    const msg=document.getElementById('mic_msg');
    const audio=document.getElementById('mic_audio');
    const down=document.getElementById('mic_download');
    msg.textContent='Gravando 3 segundos no MS3625... fale agora.';
    const controller=new AbortController();
    const timer=setTimeout(()=>controller.abort(),15000);
    let response;
    try{
      response=await fetch('/mic/record?ip='+encodeURIComponent(ip)+'&seconds=3&t='+Date.now(),{cache:'no-store',signal:controller.signal});
    }catch(e){
      if(e.name==='AbortError') throw new Error('Tempo esgotado ao gravar o microfone.');
      throw e;
    }finally{
      clearTimeout(timer);
    }
    if(!response.ok) throw new Error((await response.text())||('Falha HTTP '+response.status));
    const blob=await response.blob();
    if(blob.size<=44) throw new Error('A placa devolveu um WAV vazio.');
    if(jrMicUrl) URL.revokeObjectURL(jrMicUrl);
    jrMicUrl=URL.createObjectURL(blob);
    audio.src=jrMicUrl;
    audio.style.display='block';
    down.href=jrMicUrl;
    down.style.display='inline-block';
    msg.textContent='Gravacao pronta: '+blob.size+' bytes. Microfone confirmado; use play para ouvir no PC.';
    localLine('JR_MIC_RECORD ip='+ip+' bytes='+blob.size+' wav=1');
    await send('status');
  },'mic_msg');
}

async function playRecordingOnSpeaker(){
  return action(async()=>{
    if(device?.voice_running==='1') throw new Error('Audio/I2S em uso pela voz. Pare autonomo/Playground.');
    const msg=document.getElementById('audio_msg');
    msg.textContent='Reproduzindo a ultima gravacao do microfone na caixinha...';
    const reply=await send('audio_play_recording');
    const f=fields(reply);
    if(f.audio_play_recording!=='completed') throw new Error('Reproducao nao confirmada.');
    msg.textContent='Gravacao reproduzida na caixinha: '+(f.seconds||'?')+'s, '+(f.samples||'?')+' amostras, volume '+(f.volume||'?')+'%.';
  },'audio_msg');
}

const jrBaseTestCamera=testCamera;
testCamera=async function(){
  if(mode==='wifi') return takePhoto();
  return jrBaseTestCamera();
};

function openCameraPortal(){
  const ip=val('esp_ip');
  if(!jrValidIpv4(ip)){el('cam_msg').textContent='Consulte o Wi-Fi da placa para obter o IP atual.';return;}
  window.open('http://'+ip+'/','_blank','noopener');
}

/* index.html ainda contem o texto historico 0.70; esta extensao roda depois dele
 * e mostra o limiar realmente compilado nesta candidata. */
const jrBaseRenderBrain=(typeof renderBrain==='function')?renderBrain:null;
if(jrBaseRenderBrain){
  renderBrain=function(text){
    jrBaseRenderBrain(text);
    const f=fields(text);
    const enabled=(f.autonomous==='1'||f.enabled==='1'||(typeof autonomousEnabled!=='undefined'&&autonomousEnabled));
    if(enabled){
      const threshold=f.min_probability||device?.voice_min_probability||'0.600';
      const rejected=f.rejected||device?.voice_rejected||'0';
      el('brain_msg').textContent='Reconhecedor: somente JR BOT / J R BOT. Confianca minima '+threshold+'. Abaixo do limite nao muda a face nem fala. Rejeitadas: '+rejected+'.';
    }else{
      el('brain_msg').textContent='Modo autonomo desligado. Playground pode medir probabilidades sem executar face ou audio.';
    }
  };
}

jrNormalizeCameraUi();
jrNormalizeAudioMicUi();
refreshControls();
