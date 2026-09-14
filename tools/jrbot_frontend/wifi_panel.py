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
  const wifiIds=['wifi_ip','wifi_gw','wifi_mask','wifi_dns1','wifi_dns2'];
  const bytes=s=>new TextEncoder().encode(s).length;
  function raw(id){return el(id).value;}
  function ipv4Number(text){
    const parts=text.trim().split('.');
    if(parts.length!==4)return null;
    let value=0;
    for(const part of parts){
      if(!/^\d{1,3}$/.test(part)||(part.length>1&&part[0]==='0'))return null;
      const n=Number(part);if(n<0||n>255)return null;
      value=((value<<8)|n)>>>0;
    }
    return value>>>0;
  }
  function unicast(value){const first=value>>>24;return value!==0&&value!==0xffffffff&&first!==127&&first>0&&first<224;}
  function validateStaticIp(){
    const ip=ipv4Number(val('wifi_ip')),gw=ipv4Number(val('wifi_gw')),mask=ipv4Number(val('wifi_mask')),d1=ipv4Number(val('wifi_dns1')),d2=ipv4Number(val('wifi_dns2'));
    if([ip,gw,mask,d1,d2].some(v=>v===null))throw new Error('IP fixo: informe IP, gateway, mascara e DNS em IPv4 valido.');
    const hosts=(~mask)>>>0;
    if(mask===0||hosts<3||((hosts&((hosts+1)>>>0))>>>0)!==0)throw new Error('IP fixo: mascara de rede invalida.');
    if(!unicast(ip)||!unicast(gw)||!unicast(d1)||!unicast(d2))throw new Error('IP fixo: IP, gateway e DNS devem ser enderecos IPv4 validos.');
    if(((ip&mask)>>>0)!==((gw&mask)>>>0)||ip===gw)throw new Error('IP fixo: IP e gateway precisam estar na mesma rede e ser diferentes.');
    if(((ip&hosts)>>>0)===0||((ip&hosts)>>>0)===hosts||((gw&hosts)>>>0)===0||((gw&hosts)>>>0)===hosts)throw new Error('IP fixo: nao use endereco de rede ou broadcast.');
  }
  function validateWifiForm(){
    const ssid=raw('wifi_ssid');
    const pass=raw('wifi_pass');
    const host=val('wifi_host')||'jrbot';
    const staticMode=val('wifi_static');
    if(!ssid.length)throw new Error('Informe o nome do Wi-Fi (SSID).');
    if(/[\x00-\x1f\x7f]/.test(ssid)||bytes(ssid)>32)throw new Error('Nome do Wi-Fi invalido: use no maximo 32 bytes e sem caracteres de controle.');
    const passLen=bytes(pass);
    if(/[\x00-\x1f\x7f]/.test(pass))throw new Error('Senha do Wi-Fi contem caractere de controle invalido.');
    if(passLen>0&&passLen<8)throw new Error('Senha do Wi-Fi invalida: redes protegidas precisam de pelo menos 8 caracteres. Para rede aberta, deixe a senha vazia.');
    if(passLen>64)throw new Error('Senha do Wi-Fi invalida: limite de 64 bytes.');
    if(passLen===64&&!/^[0-9a-fA-F]{64}$/.test(pass))throw new Error('Senha com 64 caracteres precisa ser uma chave hexadecimal WPA valida.');
    if(bytes(host)>32||!/^[A-Za-z0-9](?:[A-Za-z0-9-]{0,30}[A-Za-z0-9])?$/.test(host))throw new Error('Nome do JrBot invalido: use somente letras, numeros e hifen, sem hifen no inicio/fim.');
    if(staticMode!=='0'&&staticMode!=='1')throw new Error('Selecione DHCP ou IP fixo.');
    if(staticMode==='1')validateStaticIp();
    return {ssid,pass,host,staticMode};
  }
  function setWifiStaticUi(){
    const fixed=val('wifi_static')==='1';
    for(const id of wifiIds)el(id).disabled=!fixed;
    const note=document.querySelector('#wificfg .full small');
    if(note)note.textContent=fixed?'IP fixo ativo: revise IP, gateway, mascara e DNS antes de salvar.':'DHCP ativo: IP, gateway, mascara e DNS abaixo sao ignorados. Depois de reiniciar, consulte o IP recebido pela placa.';
  }
  window.configureWifi=function(){
    return action(async()=>{
      if(mode!=='serial')throw new Error('Use Serial USB antes de salvar Wi-Fi.');
      const cfg=validateWifiForm();
      const fixed=cfg.staticMode==='1';
      const enc=s=>encodeURIComponent(s);
      const pairs=[
        ['ssid',enc(cfg.ssid)],['pass',enc(cfg.pass)],['host',enc(cfg.host)],['static',cfg.staticMode],
        ['ip',fixed?enc(val('wifi_ip')):''],['gw',fixed?enc(val('wifi_gw')):''],['mask',fixed?enc(val('wifi_mask')):''],
        ['dns1',fixed?enc(val('wifi_dns1')):''],['dns2',fixed?enc(val('wifi_dns2')):'']
      ];
      let reply;
      try{
        reply=await send('wifi_config_pct '+pairs.map(([k,v])=>k+'='+v).join('|'));
      }catch(error){
        if(String(error&&error.message||error).includes('valores_invalidos_nao_salvos')){
          throw new Error('O firmware recusou a configuracao. Revise SSID, senha, nome do JrBot e, se estiver usando IP fixo, os dados de rede. Nenhum valor foi salvo.');
        }
        throw error;
      }
      if(!reply.startsWith('JR_WIFI_SALVO'))throw new Error('Salvamento do Wi-Fi nao foi confirmado.');
      saveWifiLocal();
      el('wifi_pass').value='';
      el('device_msg').textContent='Wi-Fi salvo'+(fixed?' com IP fixo':' em DHCP')+'. Reinicie a placa e consulte o Wi-Fi da placa.';
    });
  };
  el('wifi_static').addEventListener('change',setWifiStaticUi);
  setWifiStaticUi();
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
