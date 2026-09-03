#!/usr/bin/env python3
"""Painel unico do JrBot.
Abre http://127.0.0.1:8765 e permite controlar por Serial USB ou por Wi-Fi na mesma tela.
Serial continua sendo o modo de configuracao/diagnostico; Wi-Fi usa o IP mostrado pelo ESP32.
"""
import json
import threading
import time
import urllib.error
import urllib.parse
import urllib.request
from collections import deque
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs

try:
    import serial
    from serial.tools import list_ports
except Exception:  # pragma: no cover
    serial = None
    list_ports = None

SERIAL = None
SERIAL_LOCK = threading.Lock()
READER_THREAD = None
READER_STOP = False
BAUD = 115200
APP_VERSION = "2026-09-03 19:35 UTC"
LOGS = deque(maxlen=1200)
LOG_ID = 0


def add_log(text):
    global LOG_ID
    if text is None:
        return
    text = str(text).replace("\r", "")
    for line in text.split("\n"):
        if line == "":
            continue
        LOG_ID += 1
        LOGS.append({"id": LOG_ID, "ts": time.strftime("%H:%M:%S"), "line": line})


HTML = r"""
<!doctype html>
<html lang="pt-br">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>JrBot</title>
<style>
:root{--bg:#0b0d12;--panel:#151923;--panel2:#10131a;--line:#252b38;--txt:#eef3ff;--muted:#9aa7bd;--blue:#2f80ed;--green:#25a55f;--red:#d04b3f;--yellow:#f3b33d;--purple:#9b62f0}
*{box-sizing:border-box} body{margin:0;background:radial-gradient(circle at top,#182033,#0b0d12 48%);color:var(--txt);font-family:Inter,Segoe UI,Arial,sans-serif;min-height:100vh}
header{min-height:64px;display:flex;align-items:center;justify-content:space-between;gap:12px;padding:12px 22px;border-bottom:1px solid var(--line);background:rgba(10,12,18,.75);backdrop-filter:blur(8px)}
h1{font-size:20px;margin:0}.version{color:#b9ffd7;font-size:12px;margin-top:4px}.sub{color:var(--muted);font-size:13px;margin-top:3px}.status{display:flex;gap:10px;align-items:center;flex-wrap:wrap}.pill{padding:8px 12px;border:1px solid var(--line);border-radius:999px;background:var(--panel);font-size:13px;color:var(--muted)}.pill.ok{color:#b9ffd7;border-color:#246b43}.pill.bad{color:#ffcbc6;border-color:#74312c}
main{display:grid;grid-template-columns:1.15fr .85fr;gap:14px;padding:14px}.card{background:rgba(21,25,35,.88);border:1px solid var(--line);border-radius:18px;overflow:hidden;box-shadow:0 12px 30px rgba(0,0,0,.25)}.card h2{font-size:15px;margin:0;padding:14px 16px;border-bottom:1px solid var(--line);color:#dfe8ff;display:flex;justify-content:space-between;align-items:center}.left,.right{display:flex;flex-direction:column;min-height:0}
.toolbar{display:flex;gap:8px;align-items:center;padding:12px;border-bottom:1px solid var(--line);flex-wrap:wrap}select,input{background:#090b10;color:var(--txt);border:1px solid var(--line);border-radius:10px;padding:10px;font-size:14px}select{min-width:170px}button{border:0;border-radius:12px;padding:10px 13px;color:white;font-weight:700;cursor:pointer;background:var(--blue);transition:.12s transform,.12s opacity}button:hover{transform:translateY(-1px)}button:active{transform:translateY(0);opacity:.82}.green{background:var(--green)}.red{background:var(--red)}.gray{background:#30394d}.yellow{background:var(--yellow);color:#1c1400}.purple{background:var(--purple)}
.modebar{padding:12px;border-bottom:1px solid var(--line);display:flex;gap:10px;flex-wrap:wrap;align-items:center}.modebar input[type=radio]{accent-color:var(--blue)}.modebar label{background:#10131a;border:1px solid var(--line);border-radius:14px;padding:10px 12px;cursor:pointer}.modebar label.active{border-color:#2f80ed;color:#d8e9ff}.modebar .ip{width:160px}.modepanel{display:none;gap:8px;align-items:center;padding:12px;border-bottom:1px solid var(--line);flex-wrap:wrap}.modepanel.show{display:flex}.modepanel .ip{width:180px}.mini{color:var(--muted);font-size:12px;width:100%;margin-top:2px}
#log{height:460px;max-height:55vh;margin:0;padding:14px;background:#050609;color:#a8ffbf;font-family:Consolas,Menlo,monospace;font-size:13px;line-height:1.35;overflow-y:scroll;overflow-x:auto;white-space:pre-wrap;scroll-behavior:smooth}.logline .ts{color:#6e7890}.logline .tx{color:#7ab7ff}.logline .err{color:#ff8f86}.logline .ok{color:#a8ffbf}.hint{color:var(--muted);font-size:13px;padding:0 12px 12px}
.faces{padding:14px;display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px;overflow:auto}.face{display:flex;align-items:center;gap:10px;text-align:left;background:linear-gradient(180deg,#202739,#161b27);border:1px solid #30384b;padding:12px;border-radius:14px;min-height:68px}.face .emoji{font-size:25px;width:34px;text-align:center}.face .name{font-size:15px}.face .cmd{font-size:12px;color:var(--muted);margin-top:2px}.quick{padding:12px;border-top:1px solid var(--line);display:flex;gap:8px;flex-wrap:wrap}.custom{display:flex;gap:8px;width:100%}.custom input{flex:1}.wifi{padding:12px;border-top:1px solid var(--line);display:grid;grid-template-columns:1fr 1fr;gap:8px}.wifi label{font-size:12px;color:var(--muted)}.wifi input,.wifi select{width:100%;margin-top:4px}.wifi .full{grid-column:1/-1}.wifi small{color:var(--muted)}.camera{padding:12px;border-top:1px solid var(--line);display:flex;gap:10px;flex-wrap:wrap;align-items:center}.camera img{width:100%;max-height:420px;object-fit:contain;background:#050609;border:1px solid var(--line);border-radius:14px}.camera .msg{color:var(--muted);font-size:13px;width:100%}
@media(max-width:850px){main{grid-template-columns:1fr}.card{min-height:360px}.faces{grid-template-columns:1fr}#log{min-height:320px}.modebar .ip{width:100%}}
</style>
</head>
<body>
<header>
  <div><h1>JrBot</h1><div class="version">Versão: {APP_VERSION}</div><div class="sub">Mesmo painel: Serial USB ou Wi-Fi mudam só o modo de conexão</div></div>
  <div class="status"><span id="conn" class="pill bad">desconectado</span><span id="last" class="pill">sem log</span></div>
</header>
<main>
  <section class="card left">
    <h2>Conexão e log <span><button class="gray" onclick="clearLog()">limpar</button></span></h2>
    <div class="modebar">
      <label id="lbl_serial"><input type="radio" name="mode" value="serial" checked onchange="setMode('serial')"> Serial USB</label>
      <label id="lbl_wifi"><input type="radio" name="mode" value="wifi" onchange="setMode('wifi')"> Wi-Fi</label>
      <button onclick="openWifiConfig()" class="yellow">Configurar Wi-Fi</button>
    </div>
    <div class="modepanel show" id="serialbar">
      <select id="port"></select>
      <button onclick="refreshPorts()" class="gray">Atualizar portas</button>
      <button onclick="connect()" class="green">Conectar Serial</button>
      <button onclick="disconnect()" class="red">Desconectar</button>
      <div class="mini">Modo Serial: lê e envia comandos pela porta USB/COM do ESP32.</div>
    </div>
    <div class="modepanel" id="wifibar">
      <span>IP do ESP32:</span><input id="esp_ip" class="ip" value="192.168.0.83" placeholder="192.168.0.83">
      <button onclick="connectWifi()" class="green">Conectar Wi-Fi</button>
      <button onclick="testWifi()" class="yellow">Testar/status</button>
      <div class="mini">Modo Wi-Fi: a tela não muda; os mesmos botões passam a ler/enviar pelo IP do ESP32.</div>
    </div>
    <pre id="log"></pre>
  </section>
  <section class="card right">
    <h2>Rostos</h2>
    <div class="faces" id="faces"></div>
    <div class="quick">
      <button onclick="send('status')" class="gray">status</button>
      <button onclick="send('help')" class="gray">help</button>
      <button onclick="send('demo')" class="purple">demo on/off</button>
      <div class="custom"><input id="custom" placeholder="comando manual"><button onclick="sendCustom()">enviar</button></div>
    </div>
    <div class="hint">Use Serial para configurar/diagnosticar. Clique em Wi-Fi para controlar pelo IP mantendo esta mesma tela.</div>
    <h2>Camera</h2>
    <div class="camera" id="camera_box">
      <button onclick="takePhoto(1)" class="green">📷 Focar e tirar foto</button>
      <button onclick="takePhoto(0)" class="yellow">Tirar foto sem foco</button>
      <button onclick="openCameraPortal()" class="gray">Abrir câmera no ESP32</button>
      <div class="msg" id="cam_msg">Use no modo Wi-Fi com o IP correto do ESP32.</div>
      <img id="cam_photo" alt="Foto capturada pela câmera do JrBot" style="display:none">
    </div>
    <h2 id="wifi_config_title">Configurar Wi-Fi pelo Serial</h2>
    <div class="wifi" id="wificfg">
      <label>Nome do Wi-Fi<input id="wifi_ssid" placeholder="nome da rede"></label>
      <label>Senha<input id="wifi_pass" placeholder="senha" type="password"></label>
      <label>Nome do JrBot<input id="wifi_host" value="jrbot"></label>
      <label>IP fixo<select id="wifi_static"><option value="0">desativado/DHCP</option><option value="1">ativado</option></select></label>
      <label>IP do ESP32<input id="wifi_ip" value="192.168.0.83"></label>
      <label>Gateway<input id="wifi_gw" value="192.168.0.1"></label>
      <label>Máscara<input id="wifi_mask" value="255.255.255.0"></label>
      <label>DNS1<input id="wifi_dns1" value="8.8.8.8"></label>
      <label>DNS2<input id="wifi_dns2" value="8.8.4.4"></label>
      <div class="full">
        <button onclick="configureWifi()" class="green">Configurar Wi-Fi no ESP32</button>
        <button onclick="clearWifi()" class="red">Limpar Wi-Fi salvo</button><br>
        <small>Este bloco envia pela Serial. Depois de conectar, mude o modo no topo para Wi-Fi.</small>
      </div>
    </div>
  </section>
</main>
<script>
const faces=[
 ['🤖','Neutro','neutro'],['😊','Feliz','feliz'],['😢','Triste','triste'],['😃','Animado','animado'],
 ['😠','Bravo','bravo'],['😮','Surpreso','surpreso'],['🤔','Pensando','pensando'],['😒','Cético','cetico'],
 ['😴','Sono','sono'],['😵💫','Confuso','confuso'],['😉','Piscando','piscando'],['😍','Amor','amor'],
 ['😜','Brincalhão','brincalhao'],['😟','Preocupado','preocupado'],['😎','Cool','cool'],['🔋','Bateria baixa','bateria']
];
let lastId=0, autoScroll=true, mode=localStorage.getItem('jr_mode')||'serial';
const logEl=document.getElementById('log');
document.getElementById('faces').innerHTML=faces.map(f=>`<button class="face" onclick="send('${f[2]}')"><span class="emoji">${f[0]}</span><span><div class="name">${f[1]}</div><div class="cmd">${f[2]}</div></span></button>`).join('');
function appendLog(items){
  for(const it of items){
    const line=document.createElement('div'); line.className='logline';
    let cls='ok'; if(it.line.startsWith('>')) cls='tx'; if(it.line.includes('ERROR')||it.line.includes('erro')) cls='err';
    line.innerHTML=`<span class="ts">[${it.ts}]</span> <span class="${cls}"></span>`;
    line.querySelector('span:last-child').textContent=it.line;
    logEl.appendChild(line); lastId=Math.max(lastId,it.id); document.getElementById('last').textContent=it.ts;
  }
  if(autoScroll) logEl.scrollTop=logEl.scrollHeight;
}
function localLine(line){appendLog([{id:++lastId,ts:new Date().toLocaleTimeString(),line}])}
function clearLog(){logEl.textContent=''; lastId=0}
function openWifiConfig(){document.getElementById('wifi_config_title').scrollIntoView({behavior:'smooth',block:'start'}); localLine('Abra/edite a configuração Wi-Fi abaixo. O envio da configuração é feito pela Serial.')}
function val(id){return document.getElementById(id).value.trim()}
async function api(path, opts){const r=await fetch(path,opts); const t=await r.text(); if(!r.ok) throw new Error(t); return t}
async function setMode(m){mode=m;localStorage.setItem('jr_mode',m);document.querySelector(`input[value=${m}]`).checked=true;document.getElementById('lbl_serial').classList.toggle('active',m==='serial');document.getElementById('lbl_wifi').classList.toggle('active',m==='wifi');document.getElementById('serialbar').classList.toggle('show',m==='serial');document.getElementById('wifibar').classList.toggle('show',m==='wifi');document.getElementById('wificfg').style.display='grid';document.getElementById('conn').textContent=m==='wifi'?'modo Wi-Fi '+val('esp_ip'):'modo Serial';document.getElementById('conn').className='pill '+(m==='wifi'?'ok':'bad');if(m==='wifi'){try{await api('/disconnect',{method:'POST'});}catch(e){} localStorage.setItem('jr_esp_ip',val('esp_ip'));}localLine('modo alterado para '+(m==='wifi'?'Wi-Fi pelo IP '+val('esp_ip')+'; Serial parada':'Serial USB pela COM'))}
async function refreshPorts(){try{let r=await fetch('/ports');let j=await r.json();let s=document.getElementById('port');s.innerHTML='';j.ports.forEach(p=>{let o=document.createElement('option');o.value=p;o.textContent=p;s.appendChild(o)}); localLine('portas: '+(j.ports.join(', ')||'nenhuma'));}catch(e){alert(e)}}
async function connect(){try{let port=document.getElementById('port').value;let t=await api('/connect',{method:'POST',body:new URLSearchParams({port})});document.getElementById('conn').textContent='Serial conectado '+port;document.getElementById('conn').className='pill ok';localLine(t);}catch(e){alert(e.message)}}
async function disconnect(){try{let t=await api('/disconnect',{method:'POST'});document.getElementById('conn').textContent='Serial desconectado';document.getElementById('conn').className='pill bad';localLine(t);}catch(e){alert(e.message)}}
async function send(command){try{let params=new URLSearchParams({command,mode,ip:val('esp_ip')});let t=await api('/send',{method:'POST',body:params});localLine('> '+mode+' '+command); if(t.trim()) localLine(t.trim()); if(mode==='wifi') localStorage.setItem('jr_esp_ip',val('esp_ip'));}catch(e){alert(e.message)}}
function sendCustom(){let v=document.getElementById('custom').value.trim(); if(v) send(v)}
async function connectWifi(){await setMode('wifi'); localStorage.setItem('jr_esp_ip',val('esp_ip')); await send('status')}
async function testWifi(){await setMode('wifi'); await send('status')}
async function takePhoto(focus){
  if(mode!=='wifi') await setMode('wifi');
  const ip=val('esp_ip');
  const photo=document.getElementById('cam_photo');
  const msg=document.getElementById('cam_msg');
  msg.textContent=focus?'Focando e capturando foto...':'Capturando foto...';
  try{
    const url='/camera/capture?ip='+encodeURIComponent(ip)+'&focus='+(focus?1:0)+'&t='+Date.now();
    const r=await fetch(url);
    if(!r.ok) throw new Error(await r.text());
    const blob=await r.blob();
    if(photo.dataset.url) URL.revokeObjectURL(photo.dataset.url);
    const obj=URL.createObjectURL(blob);
    photo.dataset.url=obj;
    photo.src=obj;
    photo.style.display='block';
    msg.textContent='Foto capturada pela câmera do JrBot.';
    localLine('camera: foto capturada via Wi-Fi '+ip);
  }catch(e){msg.textContent='Erro ao capturar foto: '+e.message; localLine('JR_CAMERA_ERROR '+e.message)}
}
function openCameraPortal(){const ip=val('esp_ip')||'192.168.0.83'; window.open('http://'+ip.replace(/^https?:\/\//,'').replace(/\/$/,'')+'/', '_blank')}
function saveWifiLocal(){['wifi_ssid','wifi_host','wifi_static','wifi_ip','wifi_gw','wifi_mask','wifi_dns1','wifi_dns2','esp_ip'].forEach(id=>localStorage.setItem('jr_'+id,val(id)))}
function loadWifiLocal(){['wifi_ssid','wifi_host','wifi_static','wifi_ip','wifi_gw','wifi_mask','wifi_dns1','wifi_dns2','esp_ip'].forEach(id=>{let v=localStorage.getItem('jr_'+id); if(v!==null) document.getElementById(id).value=v})}
function cleanWifiValue(v){return (v||'').replace(/[|\r\n]/g,' ').trim()}
async function configureWifi(){
  let ssid=cleanWifiValue(val('wifi_ssid'));
  if(!ssid){alert('Informe o nome do Wi-Fi');return}
  await setMode('serial');
  let cmd='wifi_config ssid='+ssid+'|pass='+cleanWifiValue(document.getElementById('wifi_pass').value)+'|host='+cleanWifiValue(val('wifi_host')||'jrbot')+'|static='+val('wifi_static')+'|ip='+cleanWifiValue(val('wifi_ip'))+'|gw='+cleanWifiValue(val('wifi_gw'))+'|mask='+cleanWifiValue(val('wifi_mask'))+'|dns1='+cleanWifiValue(val('wifi_dns1'))+'|dns2='+cleanWifiValue(val('wifi_dns2'));
  saveWifiLocal();
  await send(cmd);
  localLine('Aguarde JR_WIFI conectado ip=...; copie esse IP no campo IP Wi-Fi e mude para modo Wi-Fi.');
}
async function clearWifi(){if(confirm('Limpar Wi-Fi salvo no ESP32?')) await send('wifi_clear')}
document.getElementById('custom').addEventListener('keydown',e=>{if(e.key==='Enter')sendCustom()});
logEl.addEventListener('scroll',()=>{autoScroll=(logEl.scrollTop+logEl.clientHeight>=logEl.scrollHeight-20)});
async function poll(){try{let r=await fetch('/logs?after='+lastId);let j=await r.json(); if(j.serial_connected&&mode==='serial'){document.getElementById('conn').textContent='Serial conectado';document.getElementById('conn').className='pill ok'} if(j.logs.length) appendLog(j.logs);}catch(e){} finally{setTimeout(poll,600)}}
loadWifiLocal(); setMode(mode); refreshPorts(); poll();
</script>
</body>
</html>
"""


def reader_loop():
    global READER_STOP, SERIAL
    while not READER_STOP:
        try:
            with SERIAL_LOCK:
                ser = SERIAL
            if ser and ser.is_open:
                data = ser.read(512)
                if data:
                    add_log(data.decode("utf-8", errors="replace"))
            else:
                time.sleep(0.2)
        except Exception as exc:
            msg = str(exc)
            if "PermissionError" in msg or "Acesso negado" in msg or "ClearCommError" in msg:
                add_log("JR_PANEL_SERIAL_OFF porta Serial liberada/indisponivel; se estiver em modo Wi-Fi isso e normal")
                with SERIAL_LOCK:
                    try:
                        if SERIAL:
                            SERIAL.close()
                    except Exception:
                        pass
                    SERIAL = None
                time.sleep(1.0)
            else:
                add_log(f"JR_PANEL_ERROR leitura_serial: {exc}")
                time.sleep(0.5)


def ensure_reader():
    global READER_THREAD, READER_STOP
    if READER_THREAD and READER_THREAD.is_alive():
        return
    READER_STOP = False
    READER_THREAD = threading.Thread(target=reader_loop, daemon=True)
    READER_THREAD.start()


def close_serial(reason="Serial fechada"):
    global SERIAL
    with SERIAL_LOCK:
        if SERIAL:
            try:
                SERIAL.close()
            except Exception:
                pass
            SERIAL = None
    add_log(reason)


def wifi_request(ip, command):
    ip = (ip or "").strip() or "192.168.0.83"
    safe_ip = ip.replace("http://", "").replace("https://", "").strip("/")
    url = f"http://{safe_ip}/cmd?c=" + urllib.parse.quote(command)
    try:
        with urllib.request.urlopen(url, timeout=4) as resp:
            return resp.read().decode("utf-8", errors="replace")
    except urllib.error.URLError as exc:
        raise RuntimeError(f"falha Wi-Fi em http://{safe_ip}/ : {exc}") from exc


def camera_capture_request(ip, focus):
    ip = (ip or "").strip() or "192.168.0.83"
    safe_ip = ip.replace("http://", "").replace("https://", "").strip("/")
    focus_value = "1" if str(focus).strip().lower() in ("1", "true", "sim") else "0"
    url = f"http://{safe_ip}/capture?focus={focus_value}&t={int(time.time() * 1000)}"
    try:
        with urllib.request.urlopen(url, timeout=12) as resp:
            ctype = resp.headers.get("Content-Type", "image/jpeg")
            data = resp.read()
            if not data:
                raise RuntimeError("camera retornou imagem vazia")
            return data, ctype
    except urllib.error.URLError as exc:
        raise RuntimeError(f"falha ao capturar camera em http://{safe_ip}/capture : {exc}") from exc


class Handler(BaseHTTPRequestHandler):
    def _send(self, code=200, body="", ctype="text/plain; charset=utf-8"):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        if isinstance(body, str):
            body = body.encode("utf-8")
        self.wfile.write(body)

    def do_GET(self):
        if self.path == "/" or self.path.startswith("/?"):
            self._send(200, HTML.replace("{APP_VERSION}", APP_VERSION), "text/html; charset=utf-8")
        elif self.path == "/ports":
            ports = [] if list_ports is None else [p.device for p in list_ports.comports()]
            self._send(200, json.dumps({"ports": ports}), "application/json")
        elif self.path.startswith("/camera/capture"):
            params = {}
            if "?" in self.path:
                params = parse_qs(self.path.split("?", 1)[1])
            try:
                img, ctype = camera_capture_request(params.get("ip", ["192.168.0.83"])[0], params.get("focus", ["1"])[0])
            except RuntimeError as exc:
                self._send(502, str(exc))
                return
            add_log("JR_CAMERA foto capturada pelo painel")
            self._send(200, img, ctype)
        elif self.path.startswith("/logs"):
            after = 0
            if "?" in self.path:
                params = parse_qs(self.path.split("?", 1)[1])
                try:
                    after = int(params.get("after", ["0"])[0])
                except ValueError:
                    after = 0
            with SERIAL_LOCK:
                connected = bool(SERIAL and SERIAL.is_open)
            items = [x for x in list(LOGS) if x["id"] > after]
            self._send(200, json.dumps({"serial_connected": connected, "logs": items[-250:]}), "application/json")
        else:
            self._send(404, "not found")

    def do_POST(self):
        global SERIAL
        length = int(self.headers.get("Content-Length", "0"))
        data = parse_qs(self.rfile.read(length).decode("utf-8"))
        if self.path == "/connect":
            if serial is None:
                self._send(500, "pyserial nao instalado. Rode: pip install -r requirements.txt")
                return
            port = data.get("port", [""])[0]
            if not port:
                self._send(400, "porta vazia")
                return
            with SERIAL_LOCK:
                if SERIAL:
                    SERIAL.close()
                SERIAL = serial.Serial(port, BAUD, timeout=0.1)
            ensure_reader()
            add_log(f"JR_PANEL conectado em {port} @ {BAUD}")
            self._send(200, f"conectado em {port} @ {BAUD}")
        elif self.path == "/disconnect":
            with SERIAL_LOCK:
                if SERIAL:
                    SERIAL.close()
                SERIAL = None
            add_log("JR_PANEL desconectado")
            self._send(200, "desconectado")
        elif self.path == "/send":
            command = data.get("command", [""])[0].strip()
            mode = data.get("mode", ["serial"])[0].strip().lower()
            ip = data.get("ip", ["192.168.0.83"])[0].strip()
            if not command:
                self._send(400, "comando vazio")
                return
            if mode == "wifi":
                try:
                    response = wifi_request(ip, command)
                except RuntimeError as exc:
                    self._send(502, str(exc))
                    return
                add_log(f"> wifi {ip} {command}")
                add_log(response)
                self._send(200, response)
                return
            if serial is None:
                self._send(500, "pyserial nao instalado. Rode: pip install -r requirements.txt")
                return
            with SERIAL_LOCK:
                if not SERIAL or not SERIAL.is_open:
                    self._send(400, "nao conectado na Serial")
                    return
                SERIAL.write((command + "\n").encode("utf-8"))
                SERIAL.flush()
            add_log("> " + command)
            self._send(200, "enviado")
        else:
            self._send(404, "not found")

    def log_message(self, fmt, *args):
        return


if __name__ == "__main__":
    host, port = "127.0.0.1", 8765
    add_log("JR_PANEL unico iniciado")
    print(f"JrBot painel unico: http://{host}:{port}")
    print("Ctrl+C para sair")
    ThreadingHTTPServer((host, port), Handler).serve_forever()
