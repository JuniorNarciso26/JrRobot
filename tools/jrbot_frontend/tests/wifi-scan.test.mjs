import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';
import test from 'node:test';

const python=fs.readFileSync(new URL('../wifi_panel.py',import.meta.url),'utf8');
const script=python.match(/_WIFI_SCRIPT = r"""\s*<script>([\s\S]*?)<\/script>/)[1];

function setup(){
  const elements=new Map();
  const el=id=>{
    if(!elements.has(id))elements.set(id,{
      value:'',disabled:false,textContent:'',children:[],listeners:{},
      replaceChildren(){this.children=[];this.value='';},
      appendChild(child){this.children.push(child);},
      addEventListener(name,fn){this.listeners[name]=fn;},focus(){},
    });
    return elements.get(id);
  };
  el('wifi_static').value='0';el('wifi_host').value='jrbot';
  let reply='',commands=[],profiles=[],useReply='JR_WIFI_SALVO pending_restart=1';
  const context={el,TextEncoder,mode:'serial',serialConnected:true,window:{},
    val:id=>el(id).value.trim(),action:fn=>fn(),saveWifiLocal(){},
    document:{createElement:()=>({}),querySelector:()=>null},
    confirm:()=>true,
    send:async command=>{
      commands.push(command);if(reply instanceof Error)throw reply;
      if(command==='wifi_profiles')return 'JR_WIFI_PROFILES '+JSON.stringify({profiles});
      if(command.startsWith('wifi_profile_delete ')){profiles=profiles.filter(p=>p.id!==Number(command.split(' ')[1]));return 'JR_WIFI_PROFILE_DELETED';}
      if(command.startsWith('wifi_profile_save_pct '))return 'JR_WIFI_PROFILE_SAVED';
      if(command.startsWith('wifi_profile_use '))return useReply;
      return command==='wifi_scan'?reply:'JR_WIFI_SALVO pending_restart=1';
    },
  };
  vm.runInNewContext(script,context);
  return {context,el,commands,reply(value){reply=value;},profiles(value){profiles=value;},useReply(value){useReply=value;}};
}
const entry=(ssid,rssi=-45,authmode=4)=>({ssid_pct:[...new TextEncoder().encode(ssid)].map(b=>'%'+b.toString(16).padStart(2,'0')).join(''),rssi,authmode});
const reply=networks=>'JR_WIFI_SCAN '+JSON.stringify({networks,total:networks.length,limited:false});

test('selection preserves special SSIDs, deduplicates, and requires a protected password',async()=>{
  const s=setup();s.reply(reply([entry('Rede "&<> café',-70),entry('Rede "&<> café',-42),entry('Aberta',-50,0)]));
  await s.context.window.scanWifiNetworks();
  assert.equal(s.el('wifi_networks').children.length,3);
  assert.match(s.el('wifi_networks').children[1].textContent,/Rede "&<> café \| -42/);
  s.el('wifi_pass').value='old-password';
  s.el('wifi_networks').value='0';s.el('wifi_networks').listeners.change();
  assert.equal(s.el('wifi_ssid').value,'Rede "&<> café');
  assert.equal(s.el('wifi_pass').value,'');
  await assert.rejects(s.context.window.configureWifi(),/Informe a senha/);
  assert.deepEqual(s.commands,['wifi_scan']);
  s.el('wifi_pass').value='password123';
  await s.context.window.configureWifi();
  assert.ok(s.commands[1].startsWith('wifi_config_pct ssid=Rede%20%22%26%3C%3E%20caf%C3%A9|'));
  assert.equal(s.el('wifi_pass').value,'');
});

test('empty scans and failures disable old selections without changing the configured SSID',async()=>{
  const s=setup();s.el('wifi_ssid').value='Atual';
  s.reply(reply([]));await s.context.window.scanWifiNetworks();
  assert.equal(s.el('wifi_networks').disabled,true);
  assert.equal(s.el('wifi_ssid').value,'Atual');
  s.reply(new Error('JR_WIFI_ERROR scan=busy'));
  await assert.rejects(s.context.window.scanWifiNetworks(),/Pare a Live/);
  assert.equal(s.el('wifi_networks').disabled,true);
  assert.equal(s.el('wifi_ssid').value,'Atual');
  s.context.mode='wifi';
  await assert.rejects(s.context.window.scanWifiNetworks(),/Serial USB/);
});

test('open networks can be saved without a password',async()=>{
  const s=setup();s.reply(reply([entry('Aberta',-48,0)]));
  await s.context.window.scanWifiNetworks();
  s.el('wifi_networks').value='0';s.el('wifi_networks').listeners.change();
  await s.context.window.configureWifi();
  assert.match(s.commands[1],/ssid=Aberta\|pass=\|/);
});

test('using a saved network sends only its id and reuses the stored password',async()=>{
  const s=setup();s.profiles([{id:1,...entry('Saved'),mode:'DHCP',selected:false,protected:true}]);
  await s.context.window.refreshWifiProfiles();s.el('wifi_profiles').value='1';
  await s.context.window.useWifiProfile();
  assert.ok(s.commands.includes('wifi_profile_use 1'));
  assert.ok(!s.commands.some(c=>c.includes('pass=')));
  assert.match(s.el('wifi_profiles_msg').textContent,/senha salva.*Reinicie/);
  s.el('wifi_ssid').value='Saved';
  await assert.rejects(s.context.window.configureWifi(),/Use Usar rede salva/);
});

test('saving another profile does not select it, and deleting uses its id',async()=>{
  const s=setup();s.el('wifi_ssid').value='Other';s.el('wifi_pass').value='password123';
  await s.context.window.saveWifiProfile();
  assert.ok(s.commands[0].startsWith('wifi_profile_save_pct '));
  assert.ok(!s.commands.some(c=>c.startsWith('wifi_profile_use ')));
  assert.equal(s.el('wifi_pass').value,'');
  assert.match(s.el('wifi_profiles_msg').textContent,/conexao atual foi mantida/);
  s.profiles([{id:2,...entry('Other'),mode:'DHCP',selected:false,protected:true}]);
  await s.context.window.refreshWifiProfiles();s.el('wifi_profiles').value='2';
  await s.context.window.deleteWifiProfile();
  assert.ok(s.commands.includes('wifi_profile_delete 2'));
  assert.equal(s.el('wifi_profiles').disabled,true);
});

test('using the current configuration does not ask for restart',async()=>{
  const s=setup();s.profiles([{id:0,...entry('Current'),mode:'DHCP',selected:true,protected:true}]);
  s.useReply('JR_WIFI_SALVO pending_restart=0 configuracao_ja_ativa');
  await s.context.window.refreshWifiProfiles();s.el('wifi_profiles').value='0';
  await s.context.window.useWifiProfile();
  assert.match(s.el('wifi_profiles_msg').textContent,/Nao precisa reiniciar/);
});

test('scan distinguishes no initialized Wi-Fi from waiting for connection/IP',async()=>{
  const s=setup();s.reply(new Error('JR_WIFI_ERROR scan=not_started'));
  await assert.rejects(s.context.window.scanWifiNetworks(),/ainda nao foi iniciado/);
  s.reply(new Error('JR_WIFI_ERROR scan=not_connected'));
  await assert.rejects(s.context.window.scanWifiNetworks(),/ainda nao recebeu um IP/);
});
