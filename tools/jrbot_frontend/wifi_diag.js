'use strict';
(function(){
  function ensureBox(){
    let box=el('wifi_board_saved');
    if(box)return box;
    box=document.createElement('div');
    box.id='wifi_board_saved';
    box.className='full';
    box.style.cssText='margin-top:8px;padding:11px;border:1px solid #30384b;border-radius:12px;background:#10131a;color:#dfe8ff;white-space:pre-wrap;font:13px/1.5 Consolas,monospace';
    box.textContent='Diagnostico da rede: ainda nao consultado.';
    el('wificfg').appendChild(box);
    return box;
  }

  function parseApi(text){
    if(!text.startsWith('JR_API '))throw new Error('Resposta de diagnostico fora do protocolo.');
    let body;
    try{body=JSON.parse(text.slice(7));}catch(_){throw new Error('Resposta de diagnostico invalida.');}
    const value=body&&body.ok&&body.result&&body.result.value;
    if(!value||typeof value!=='object')throw new Error('Firmware nao retornou os dados de rede esperados.');
    return value;
  }

  function yes(v){return v?'sim':'nao';}
  function text(v,fallback='?'){return v===undefined||v===null||v===''?fallback:String(v);}

  function renderDiag(w){
    ensureBox().textContent=[
      'CONFIGURACAO GRAVADA NA PLACA',
      'Registro encontrado: '+yes(!!w.saved_record_found),
      'Registro valido: '+yes(!!w.saved_record_valid),
      'Origem: '+text(w.source),
      'SSID: '+text(w.ssid,'(vazio)'),
      'Nome do JrBot: '+text(w.hostname),
      'Modo: '+text(w.mode),
      'IP configurado: '+text(w.configured_ip),
      'Gateway: '+text(w.gateway),
      'Mascara: '+text(w.mask),
      'DNS1: '+text(w.dns1),
      'DNS2: '+text(w.dns2),
      '',
      'ESTADO ATUAL DA REDE',
      'Rede inicializada: '+yes(!!w.network_ready),
      'Conectado: '+yes(!!w.connected),
      'Reinicio pendente: '+yes(!!w.pending_restart),
      'IP atual: '+text(w.runtime_ip,'sem wifi'),
      'Ultimo erro interno: '+text(w.last_error),
      '',
      'DIAGNOSTICO DA CONEXAO',
      'Monitor de desconexao: '+yes(!!w.event_watch_active),
      'Desconexoes observadas: '+text(w.disconnect_events,'0'),
      'Ultimo motivo: '+text(w.disconnect_reason)+' ('+text(w.disconnect_reason_code,'0')+')',
      'AP conectado: '+yes(!!w.ap_available),
      'BSSID: '+text(w.bssid,'-'),
      'Canal: '+text(w.channel,'0'),
      'RSSI: '+text(w.rssi,'0')+' dBm',
      'Auth mode: '+text(w.authmode,'0'),
      '',
      'A credencial da rede nunca e exibida no painel nem no log.'
    ].join('\n');
  }

  function logDiag(w){
    localLine('JR_WIFI_PLACA_CONFIG found='+(w.saved_record_found?1:0)+' valid='+(w.saved_record_valid?1:0)+' source='+text(w.source)+' ssid="'+text(w.ssid,'')+'" host='+text(w.hostname)+' mode='+text(w.mode)+' cfg_ip='+text(w.configured_ip)+' gateway='+text(w.gateway)+' mask='+text(w.mask)+' dns1='+text(w.dns1)+' dns2='+text(w.dns2));
    localLine('JR_WIFI_PLACA_RUNTIME network_ready='+(w.network_ready?1:0)+' connected='+(w.connected?1:0)+' pending_restart='+(w.pending_restart?1:0)+' runtime_ip="'+text(w.runtime_ip,'sem wifi')+'" last_error='+text(w.last_error));
    localLine('JR_WIFI_PLACA_LINK watch='+(w.event_watch_active?1:0)+' disconnect_events='+text(w.disconnect_events,'0')+' reason_code='+text(w.disconnect_reason_code,'0')+' reason='+text(w.disconnect_reason)+' ap='+(w.ap_available?1:0)+' bssid='+text(w.bssid,'-')+' channel='+text(w.channel,'0')+' rssi='+text(w.rssi,'0')+' authmode='+text(w.authmode,'0'));
  }

  async function readBoard(){
    const reply=await send('api {"v":1,"fn":"get","args":{"path":"wifi.diagnostics"}}');
    const w=parseApi(reply);
    renderDiag(w);
    logDiag(w);
    if(w.connected&&/^\d{1,3}(?:\.\d{1,3}){3}$/.test(String(w.runtime_ip||''))){
      el('esp_ip').value=w.runtime_ip;
      localStorage.setItem('jr_esp_ip',w.runtime_ip);
    }
    return w;
  }

  function install(){
    ensureBox();

    window.wifiBoardData=function(){
      return action(async()=>{
        if(mode!=='serial')throw new Error('Use a conexao Serial para ler a configuracao gravada diretamente na placa.');
        const w=await readBoard();
        el('device_msg').textContent='Diagnostico lido da placa: SSID='+(w.ssid||'(vazio)')+', modo='+(w.mode||'?')+', IP configurado='+(w.configured_ip||'?')+', conectado='+(w.connected?'sim':'nao')+', IP atual='+(w.runtime_ip||'sem wifi')+'.';
      });
    };

    const previousConnect=window.connect;
    if(typeof previousConnect==='function'){
      window.connect=async function(){
        const result=await previousConnect();
        if(mode==='serial'&&serialConnected){
          try{await readBoard();}catch(e){localLine('JR_WIFI_DIAG_AUTO error="'+String(e.message||e)+'"');}
          setTimeout(async()=>{
            if(mode==='serial'&&serialConnected&&!busy){
              try{await readBoard();}catch(_){ }
            }
          },2500);
        }
        return result;
      };
    }
  }

  if(document.readyState==='complete')install();
  else window.addEventListener('load',install,{once:true});
})();
