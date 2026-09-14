'use strict';
(function(){
  function ensureBox(){
    let box=el('wifi_board_saved');
    if(box)return box;
    box=document.createElement('div');
    box.id='wifi_board_saved';
    box.className='full';
    box.style.cssText='margin-top:8px;padding:11px;border:1px solid #30384b;border-radius:12px;background:#10131a;color:#dfe8ff;white-space:pre-wrap;font:13px/1.5 Consolas,monospace';
    box.textContent='Dados salvos na placa: ainda nao consultados.';
    el('wificfg').appendChild(box);
    return box;
  }

  function parseWifiApi(text){
    if(!text.startsWith('JR_API '))throw new Error('Resposta de diagnostico Wi-Fi fora do protocolo.');
    let body;
    try{body=JSON.parse(text.slice(7));}catch(_){throw new Error('Resposta de diagnostico Wi-Fi invalida.');}
    const value=body&&body.ok&&body.result&&body.result.value;
    if(!value||typeof value!=='object')throw new Error('Firmware nao retornou os dados de Wi-Fi esperados.');
    return value;
  }

  async function readWifiBoard(){
    const text=await send('api {"v":1,"fn":"get","args":{"path":"wifi.status"}}');
    const w=parseWifiApi(text);
    const ssid=typeof w.ssid==='string'?w.ssid:'';
    const ip=typeof w.ip==='string'?w.ip:'sem wifi';
    ensureBox().textContent=[
      'Dados lidos da placa:',
      'SSID: '+(ssid||'(vazio)'),
      'Configurado: '+(w.configured?'sim':'nao'),
      'Conectado: '+(w.connected?'sim':'nao'),
      'Rede inicializada: '+(w.network_ready?'sim':'nao'),
      'Reinicio pendente: '+(w.pending_restart?'sim':'nao'),
      'IP atual: '+ip,
      'Ultimo erro: '+(w.last_error||'?'),
      'Chave da rede: protegida; nao exibida no painel nem no log'
    ].join('\n');
    localLine('JR_WIFI_PLACA ssid="'+ssid+'" configured='+(w.configured?1:0)+' connected='+(w.connected?1:0)+' pending_restart='+(w.pending_restart?1:0)+' ip="'+ip+'" last_error='+(w.last_error||'?'));
    if(ssid)el('wifi_ssid').value=ssid;
    if(w.connected&&/^\d{1,3}(?:\.\d{1,3}){3}$/.test(ip)){
      el('esp_ip').value=ip;
      localStorage.setItem('jr_esp_ip',ip);
    }
    return w;
  }

  window.wifiBoardData=function(){
    return action(async()=>{
      if(mode!=='serial')throw new Error('Use a conexao Serial para ler a configuracao realmente carregada na placa.');
      const w=await readWifiBoard();
      el('device_msg').textContent='Wi-Fi lido da placa: SSID='+(w.ssid||'(vazio)')+', conectado='+(w.connected?'sim':'nao')+', IP='+(w.ip||'sem wifi')+'.';
    });
  };

  const previousConfigure=window.configureWifi;
  window.configureWifi=function(){
    const fixed=val('wifi_static')==='1';
    const ssid=el('wifi_ssid').value;
    const host=val('wifi_host')||'jrbot';
    localLine('JR_WIFI_ENVIANDO ssid="'+ssid+'" host="'+host+'" mode='+(fixed?'IP_FIXO':'DHCP')+(fixed?' ip='+val('wifi_ip')+' gateway='+val('wifi_gw')+' mask='+val('wifi_mask')+' dns1='+val('wifi_dns1')+' dns2='+val('wifi_dns2'):''));
    ensureBox().textContent='Dados enviados pelo painel:\nSSID: '+(ssid||'(vazio)')+'\nNome do JrBot: '+host+'\nModo: '+(fixed?'IP fixo':'DHCP')+'\n\nAguarde no log a confirmacao JR_WIFI_SALVO. Depois reinicie e clique em Consultar Wi-Fi da placa.';
    return previousConfigure();
  };

  ensureBox();
})();
