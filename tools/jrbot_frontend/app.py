#!/usr/bin/env python3
"""Painel local do JrBot.
Abre http://127.0.0.1:8765 para conectar na Serial, ver logs ao vivo e enviar rostos ao ESP32.
Instale dependencia: pip install -r requirements.txt
"""
import json
import threading
import time
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
APP_VERSION = "2026-09-01 11:46 UTC"
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
*{box-sizing:border-box} body{margin:0;background:radial-gradient(circle at top,#182033,#0b0d12 48%);color:var(--txt);font-family:Inter,Segoe UI,Arial,sans-serif;height:100vh;overflow:hidden}
header{height:64px;display:flex;align-items:center;justify-content:space-between;padding:0 22px;border-bottom:1px solid var(--line);background:rgba(10,12,18,.75);backdrop-filter:blur(8px)}
h1{font-size:20px;margin:0}.version{color:#b9ffd7;font-size:12px;margin-top:4px}.sub{color:var(--muted);font-size:13px;margin-top:3px}.status{display:flex;gap:10px;align-items:center}.pill{padding:8px 12px;border:1px solid var(--line);border-radius:999px;background:var(--panel);font-size:13px;color:var(--muted)}.pill.ok{color:#b9ffd7;border-color:#246b43}.pill.bad{color:#ffcbc6;border-color:#74312c}
main{display:grid;grid-template-columns:1.15fr .85fr;gap:14px;height:calc(100vh - 64px);padding:14px}.card{background:rgba(21,25,35,.88);border:1px solid var(--line);border-radius:18px;overflow:hidden;box-shadow:0 12px 30px rgba(0,0,0,.25)}.card h2{font-size:15px;margin:0;padding:14px 16px;border-bottom:1px solid var(--line);color:#dfe8ff;display:flex;justify-content:space-between;align-items:center}.left,.right{display:flex;flex-direction:column;min-height:0}
.toolbar{display:flex;gap:8px;align-items:center;padding:12px;border-bottom:1px solid var(--line);flex-wrap:wrap}select,input{background:#090b10;color:var(--txt);border:1px solid var(--line);border-radius:10px;padding:10px;font-size:14px}select{min-width:170px}button{border:0;border-radius:12px;padding:10px 13px;color:white;font-weight:700;cursor:pointer;background:var(--blue);transition:.12s transform,.12s opacity}button:hover{transform:translateY(-1px)}button:active{transform:translateY(0);opacity:.82}.green{background:var(--green)}.red{background:var(--red)}.gray{background:#30394d}.yellow{background:var(--yellow);color:#1c1400}.purple{background:var(--purple)}
#log{flex:1;margin:0;padding:14px;background:#050609;color:#a8ffbf;font-family:Consolas,Menlo,monospace;font-size:13px;line-height:1.35;overflow:auto;white-space:pre-wrap}.logline .ts{color:#6e7890}.logline .tx{color:#7ab7ff}.logline .err{color:#ff8f86}.logline .ok{color:#a8ffbf}.hint{color:var(--muted);font-size:13px;padding:0 12px 12px}
.faces{padding:14px;display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px;overflow:auto}.face{display:flex;align-items:center;gap:10px;text-align:left;background:linear-gradient(180deg,#202739,#161b27);border:1px solid #30384b;padding:12px;border-radius:14px;min-height:68px}.face .emoji{font-size:25px;width:34px;text-align:center}.face .name{font-size:15px}.face .cmd{font-size:12px;color:var(--muted);margin-top:2px}.quick{padding:12px;border-top:1px solid var(--line);display:flex;gap:8px;flex-wrap:wrap}.custom{display:flex;gap:8px;width:100%}.custom input{flex:1}
@media(max-width:850px){body{overflow:auto;height:auto}main{grid-template-columns:1fr;height:auto}.card{min-height:360px}.faces{grid-template-columns:1fr}#log{min-height:380px}}
</style>
</head>
<body>
<header>
  <div><h1>JrBot</h1><div class="version">Versão: {APP_VERSION}</div><div class="sub">Painel local do robô: log serial + seleção de rostos | Wi-Fi no ESP32: http://IP_DO_ESP32/</div></div>
  <div class="status"><span id="conn" class="pill bad">desconectado</span><span id="last" class="pill">sem log</span></div>
</header>
<main>
  <section class="card left">
    <h2>Log serial <span><button class="gray" onclick="clearLog()">limpar</button></span></h2>
    <div class="toolbar">
      <select id="port"></select>
      <button onclick="refreshPorts()" class="gray">Atualizar portas</button>
      <button onclick="connect()" class="green">Conectar</button>
      <button onclick="disconnect()" class="red">Desconectar</button>
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
    <div class="hint">Dica: normalmente grava em COM6 e usa log/comandos em COM4, mas escolha a porta que aparecer no Windows.</div>
  </section>
</main>
<script>
const faces=[
 ['🤖','Neutro','neutro'],['😊','Feliz','feliz'],['😢','Triste','triste'],['😃','Animado','animado'],
 ['😠','Bravo','bravo'],['😮','Surpreso','surpreso'],['🤔','Pensando','pensando'],['😒','Cético','cetico'],
 ['😴','Sono','sono'],['😵💫','Confuso','confuso'],['😉','Piscando','piscando'],['😍','Amor','amor'],
 ['😜','Brincalhão','brincalhao'],['😟','Preocupado','preocupado'],['😎','Cool','cool'],['🔋','Bateria baixa','bateria']
];
let lastId=0, autoScroll=true;
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
function clearLog(){logEl.textContent=''; lastId=0}
async function api(path, opts){const r=await fetch(path,opts); const t=await r.text(); if(!r.ok) throw new Error(t); return t}
async function refreshPorts(){try{let r=await fetch('/ports');let j=await r.json();let s=document.getElementById('port');s.innerHTML='';j.ports.forEach(p=>{let o=document.createElement('option');o.value=p;o.textContent=p;s.appendChild(o)}); appendLog([{id:++lastId,ts:new Date().toLocaleTimeString(),line:'portas: '+(j.ports.join(', ')||'nenhuma')}]);}catch(e){alert(e)}}
async function connect(){try{let port=document.getElementById('port').value;let t=await api('/connect',{method:'POST',body:new URLSearchParams({port})});document.getElementById('conn').textContent='conectado '+port;document.getElementById('conn').className='pill ok';appendLog([{id:++lastId,ts:new Date().toLocaleTimeString(),line:t}]);}catch(e){alert(e.message)}}
async function disconnect(){try{let t=await api('/disconnect',{method:'POST'});document.getElementById('conn').textContent='desconectado';document.getElementById('conn').className='pill bad';appendLog([{id:++lastId,ts:new Date().toLocaleTimeString(),line:t}]);}catch(e){alert(e.message)}}
async function send(command){try{let t=await api('/send',{method:'POST',body:new URLSearchParams({command})});appendLog([{id:++lastId,ts:new Date().toLocaleTimeString(),line:'> '+command}]); if(t.trim()) appendLog([{id:++lastId,ts:new Date().toLocaleTimeString(),line:t.trim()}]);}catch(e){alert(e.message)}}
function sendCustom(){let v=document.getElementById('custom').value.trim(); if(v) send(v)}
document.getElementById('custom').addEventListener('keydown',e=>{if(e.key==='Enter')sendCustom()});
logEl.addEventListener('scroll',()=>{autoScroll=(logEl.scrollTop+logEl.clientHeight>=logEl.scrollHeight-20)});
async function poll(){try{let r=await fetch('/logs?after='+lastId);let j=await r.json(); if(j.connected){document.getElementById('conn').textContent='conectado';document.getElementById('conn').className='pill ok'} if(j.logs.length) appendLog(j.logs);}catch(e){} finally{setTimeout(poll,600)}}
refreshPorts(); poll();
</script>
</body>
</html>
"""


def reader_loop():
    global READER_STOP
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
            add_log(f"JR_PANEL_ERROR leitura_serial: {exc}")
            time.sleep(0.5)


def ensure_reader():
    global READER_THREAD, READER_STOP
    if READER_THREAD and READER_THREAD.is_alive():
        return
    READER_STOP = False
    READER_THREAD = threading.Thread(target=reader_loop, daemon=True)
    READER_THREAD.start()


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
            self._send(200, json.dumps({"connected": connected, "logs": items[-250:]}), "application/json")
        else:
            self._send(404, "not found")

    def do_POST(self):
        global SERIAL
        length = int(self.headers.get("Content-Length", "0"))
        data = parse_qs(self.rfile.read(length).decode("utf-8"))
        if serial is None:
            self._send(500, "pyserial nao instalado. Rode: pip install -r requirements.txt")
            return
        if self.path == "/connect":
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
            if not command:
                self._send(400, "comando vazio")
                return
            with SERIAL_LOCK:
                if not SERIAL or not SERIAL.is_open:
                    self._send(400, "nao conectado")
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
    add_log("JR_PANEL iniciado")
    print(f"JrBot painel: http://{host}:{port}")
    print("Ctrl+C para sair")
    ThreadingHTTPServer((host, port), Handler).serve_forever()
