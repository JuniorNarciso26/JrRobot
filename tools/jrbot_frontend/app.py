#!/usr/bin/env python3
"""Frontend local simples do JrBot.
Abre http://127.0.0.1:8765 para enviar comandos ao ESP32 pela Serial.
Instale dependencia: pip install -r requirements.txt
"""
import json
import sys
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import parse_qs

try:
    import serial
    from serial.tools import list_ports
except Exception:  # pragma: no cover
    serial = None
    list_ports = None

SERIAL = None
BAUD = 115200

HTML = r"""
<!doctype html><html><head><meta charset="utf-8"><title>JrBot</title>
<style>body{font-family:Arial;margin:30px;background:#111;color:#eee}button{font-size:18px;margin:6px;padding:12px 18px;border-radius:10px;border:0}select,input{font-size:16px;padding:8px}pre{background:#000;padding:12px;min-height:160px;white-space:pre-wrap}.ok{background:#2e7d32;color:white}.cmd{background:#1565c0;color:white}.bad{background:#9a3412;color:white}</style></head>
<body><h1>JrBot Face OLED</h1>
<p>Conecte na porta serial e mande comandos para os olhos.</p>
<div>Porta: <select id="port"></select> <button onclick="refreshPorts()">Atualizar</button> <button onclick="connect()" class="ok">Conectar</button></div>
<hr>
<div id="buttons"></div>
<hr>
<button onclick="send('status')" class="cmd">status</button> <button onclick="send('help')" class="cmd">help</button>
<pre id="log"></pre>
<script>
const cmds=['neutro','feliz','triste','bravo','sono','esquerda','direita','surpreso'];
document.getElementById('buttons').innerHTML=cmds.map(c=>`<button class="cmd" onclick="send('${c}')">${c}</button>`).join('');
function log(t){document.getElementById('log').textContent += t+'\n'}
async function refreshPorts(){let r=await fetch('/ports');let j=await r.json();let s=document.getElementById('port');s.innerHTML='';j.ports.forEach(p=>{let o=document.createElement('option');o.value=p;o.textContent=p;s.appendChild(o)});log('portas: '+j.ports.join(', '))}
async function connect(){let port=document.getElementById('port').value;let r=await fetch('/connect',{method:'POST',body:new URLSearchParams({port})});log(await r.text())}
async function send(command){let r=await fetch('/send',{method:'POST',body:new URLSearchParams({command})});log('> '+command+'\n'+await r.text())}
refreshPorts();
</script></body></html>
"""

class Handler(BaseHTTPRequestHandler):
    def _send(self, code=200, body="", ctype="text/plain; charset=utf-8"):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.end_headers()
        self.wfile.write(body.encode("utf-8"))

    def do_GET(self):
        if self.path == "/" or self.path.startswith("/?"):
            self._send(200, HTML, "text/html; charset=utf-8")
        elif self.path == "/ports":
            ports = [] if list_ports is None else [p.device for p in list_ports.comports()]
            self._send(200, json.dumps({"ports": ports}), "application/json")
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
            if SERIAL:
                SERIAL.close()
            SERIAL = serial.Serial(port, BAUD, timeout=0.5)
            self._send(200, f"conectado em {port} @ {BAUD}")
        elif self.path == "/send":
            if not SERIAL or not SERIAL.is_open:
                self._send(400, "nao conectado")
                return
            command = data.get("command", [""])[0].strip()
            if not command:
                self._send(400, "comando vazio")
                return
            SERIAL.write((command + "\n").encode("utf-8"))
            SERIAL.flush()
            response = SERIAL.read(1024).decode("utf-8", errors="replace")
            self._send(200, response or "enviado")
        else:
            self._send(404, "not found")

    def log_message(self, fmt, *args):
        return

if __name__ == "__main__":
    host, port = "127.0.0.1", 8765
    print(f"JrBot frontend: http://{host}:{port}")
    print("Ctrl+C para sair")
    HTTPServer((host, port), Handler).serve_forever()
