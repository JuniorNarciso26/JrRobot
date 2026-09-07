/* Camera unificada: um unico botao Testar camera usa a conexao selecionada. */
let jrPhotoUrl='';

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

const jrBaseRefreshControls=refreshControls;
refreshControls=function(){
  jrBaseRefreshControls();
  jrNormalizeCameraUi();

  const testButton=document.getElementById('test_camera');
  if(!testButton) return;

  const cameraReady=currentFirmware()&&device?.camera==='available';
  const wifiReady=cameraReady&&device?.wifi==='1'&&jrValidIpv4(val('esp_ip'));
  testButton.disabled=busy||(mode==='wifi'?!wifiReady:!cameraReady);
  testButton.textContent='Testar camera';
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

jrNormalizeCameraUi();
refreshControls();
