/* Extensao do painel: foto JPEG via Wi-Fi sem alterar o fluxo Serial que ja funciona. */
let jrPhotoUrl='';

function jrValidIpv4(text){
  return /^\d{1,3}(?:\.\d{1,3}){3}$/.test(String(text||''));
}

function jrInstallPhotoButton(){
  const box=document.getElementById('camera_box');
  if(!box) return;
  let button=document.getElementById('take_photo');
  if(!button){
    const old=Array.from(box.querySelectorAll('button')).find(b=>/previa/i.test(b.textContent||''));
    button=document.createElement('button');
    button.id='take_photo';
    button.className='yellow';
    button.setAttribute('data-action','');
    button.textContent='Ver foto pelo Wi-Fi';
    button.onclick=()=>takePhoto();
    if(old) old.replaceWith(button);
    else box.insertBefore(button,box.querySelector('.msg'));
  }
}

const jrBaseRefreshControls=refreshControls;
refreshControls=function(){
  jrBaseRefreshControls();
  jrInstallPhotoButton();
  const button=document.getElementById('take_photo');
  if(!button) return;
  const ip=val('esp_ip');
  button.disabled=busy||!currentFirmware()||device?.camera!=='available'||device?.wifi!=='1'||!jrValidIpv4(ip);
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
      ? 'OV5640 detectada. O teste Serial valida a camera; "Ver foto pelo Wi-Fi" traz o JPEG para esta tela.'
      : 'OV5640 detectada. Para ver a foto, conecte o Wi-Fi da placa e consulte o IP.';
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

    el('cam_msg').textContent='Buscando uma foto da OV5640 em '+ip+'...';
    const controller=new AbortController();
    const timer=setTimeout(()=>controller.abort(),20000);
    let response;
    try{
      response=await fetch('/camera/capture?ip='+encodeURIComponent(ip)+'&t='+Date.now(),{cache:'no-store',signal:controller.signal});
    }catch(e){
      if(e.name==='AbortError') throw new Error('Tempo esgotado ao buscar a foto pelo Wi-Fi.');
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
    el('cam_msg').textContent='Foto recebida: '+img.naturalWidth+' x '+img.naturalHeight+', '+blob.size+' bytes, IP '+ip+'.';
    localLine('JR_CAMERA_FOTO ip='+ip+' width='+img.naturalWidth+' height='+img.naturalHeight+' bytes='+blob.size+' exibida=1');
  },'cam_msg');
}

function openCameraPortal(){
  const ip=val('esp_ip');
  if(!jrValidIpv4(ip)){el('cam_msg').textContent='Consulte o Wi-Fi da placa para obter o IP atual.';return;}
  window.open('http://'+ip+'/','_blank','noopener');
}

jrInstallPhotoButton();
refreshControls();
