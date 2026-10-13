#!/usr/bin/env python3
"""Ajustes do painel de desenvolvimento para configuracao Wi-Fi segura e clara."""
from __future__ import annotations

import html
import urllib.parse

import app

_PREVIOUS_DO_GET = app.Handler.do_GET

_WIFI_SCRIPT = r"""
<script>
(function(){
  function serialOnly(){if(mode!=='serial')throw Error('Conecte o robô pelo Serial USB.');}
  function decodeName(s){try{return decodeURIComponent(s);}catch(_){return s;}}
  function dataRows(reply,prefix){if(!reply.startsWith(prefix))throw Error('Atualize o firmware para usar esta configuração Wi-Fi.');const marker=reply.indexOf(' data=');if(marker<0)throw Error('Resposta Wi-Fi incompleta.');return reply.slice(marker+6).split(';').filter(Boolean).map(r=>r.split(','));}
  async function listSaved(){const reply=await send('wifi_profiles');const rows=dataRows(reply,'JR_WIFI_PROFILES '),connected=decodeName((reply.match(/connected=([^ ]*)/)||[])[1]||'');const box=el('wifi_saved');box.replaceChildren();if(!rows.length){box.textContent='Nenhuma rede salva.';return;}for(const row of rows){const name=decodeName(row[0]),line=document.createElement('div'),label=document.createElement('span'),button=document.createElement('button');label.textContent=name+(name===connected?' · Conectada':'')+' · '+(row[1]==='1'?'Habilitada':'Desabilitada');button.textContent='Apagar';button.dataset.action='';button.onclick=()=>action(async()=>{serialOnly();const answer=await send('wifi_delete_pct '+encodeURIComponent(name));if(answer!=='JR_WIFI_DELETED')throw Error('Exclusão não confirmada.');await listSaved();el('device_msg').textContent='Rede apagada: '+name;});line.append(label,button);box.appendChild(line);}}
  window.refreshSavedWifi=()=>action(async()=>{serialOnly();await listSaved();});
  window.scanWifiNetworks=()=>action(async()=>{serialOnly();el('device_msg').textContent='Buscando redes próximas ao robô...';let reply=await send('wifi_scan');const deadline=Date.now()+20000;while(reply.includes('pending=1')){if(Date.now()>deadline)throw Error('Busca demorou demais. Tente novamente.');await new Promise(r=>setTimeout(r,500));reply=await send('wifi_scan_result');}const rows=dataRows(reply,'JR_WIFI_NETWORKS '),select=el('wifi_ssid'),previous=select.value;select.replaceChildren();const empty=document.createElement('option');empty.value='';empty.textContent=rows.length?'Selecione uma rede':'Nenhuma rede encontrada';select.appendChild(empty);const seen=new Set();for(const row of rows){const name=decodeName(row[0]);if(seen.has(name))continue;seen.add(name);const option=document.createElement('option');option.value=name;option.textContent=name+' · '+row[1]+' dBm'+(row[2]==='0'?' · Aberta':'');select.appendChild(option);}if(seen.has(previous))select.value=previous;await listSaved();el('device_msg').textContent='Selecione a rede e informe a senha.';});
  window.configureWifi=()=>action(async()=>{serialOnly();const ssid=el('wifi_ssid').value,pass=el('wifi_pass').value,bytes=s=>new TextEncoder().encode(s).length;if(!ssid||bytes(ssid)>32||/[\x00-\x1f\x7f]/.test(ssid))throw Error('Selecione uma rede válida.');const n=bytes(pass);if(/[\x00-\x1f\x7f]/.test(pass)||n>64||(n&&n<8)||(n===64&&!/^[0-9a-fA-F]{64}$/.test(pass)))throw Error('Senha inválida: use de 8 a 63 bytes ou uma chave hexadecimal de 64 caracteres. Rede aberta permite senha vazia.');el('device_msg').textContent='Conectando... aguarde até 15 segundos.';try{const reply=await send('wifi_join_pct ssid='+encodeURIComponent(ssid)+'|pass='+encodeURIComponent(pass));if(!reply.startsWith('JR_WIFI_CONNECTED '))throw Error('Conexão não confirmada.');el('wifi_pass').value='';await listSaved();el('device_msg').textContent='Conectado e salvo. Acesse https://jrbot.local na mesma rede.';}catch(e){el('wifi_pass').value='';throw Error('Não foi possível conectar. Confira a senha e tente novamente. '+e.message);}});
})();
</script>
"""


def wifi_do_get(self) -> None:
    parsed = urllib.parse.urlsplit(self.path)
    if parsed.path == "/":
        if not self._allowed():
            return
        page = (app.ROOT / "index.html").read_text(encoding="utf-8").replace("{APP_VERSION}", app.APP_VERSION)
        docs_link = '<a href="/docs/" style="padding:9px 13px;border:1px solid #3a4c68;border-radius:12px;color:#d9e7ff;text-decoration:none;font-weight:700">Documentacao</a>'
        page = page.replace("</header>", docs_link + "</header>")
        page = page.replace("</body>", '<script src="/photo_panel.js"></script>' + _WIFI_SCRIPT + "</body>")
        self._send(200, page, "text/html; charset=utf-8")
        return
    _PREVIOUS_DO_GET(self)


app.Handler.do_GET = wifi_do_get
