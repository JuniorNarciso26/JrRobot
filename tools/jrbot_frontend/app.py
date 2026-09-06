#!/usr/bin/env python3
"""JrBot V2: local panel, bounded requests and correlated Serial replies."""
from __future__ import annotations
import ipaddress
import json
import re
import threading
import time
import uuid
import urllib.error
import urllib.parse
import urllib.request
from collections import deque
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None
    list_ports = None

ROOT = Path(__file__).resolve().parent
APP_VERSION = "JRBOT-PANEL-V2-02"
MAX_COMMAND_BYTES = 768
BAUD = 115200
SERIAL = None
SERIAL_LOCK = threading.Lock()
SEND_LOCK = threading.Lock()
LOG_LOCK = threading.Lock()
PENDING_LOCK = threading.Lock()
READER_THREAD = None
READER_STOP = threading.Event()
LOGS = deque(maxlen=1200)
LOG_ID = 0
SESSION = uuid.uuid4().hex
PENDING = {}
REPLY_RE = re.compile(r"^JR_REPLY id=([A-Za-z0-9_-]{1,32}) ok=([01]) (.*)$")


def sanitize_log_line(line: str) -> str:
    text = str(line)
    # Redact the whole message, including passwords containing spaces/delimiters.
    if re.search(r"\bwifi_config(?:_pct)?(?:\s|$|%20)|(?:pass(?:word)?|senha)\s*(?:=|%3d)", text, re.I):
        return "JR_REDACTED conteudo de provisionamento omitido"
    return text


def add_log(text: str) -> None:
    global LOG_ID
    if text is None:
        return
    safe = sanitize_log_line(str(text)).replace("\r", "")
    with LOG_LOCK:
        for line in safe.split("\n"):
            if line:
                LOG_ID += 1
                LOGS.append({"id": LOG_ID, "ts": time.strftime("%H:%M:%S"), "line": line[:4096]})


def log_page(after: int, session: str = "") -> dict:
    with LOG_LOCK:
        if session and session != SESSION:
            after = 0
        first = LOGS[0]["id"] if LOGS else LOG_ID + 1
        gap = bool(after and after < first - 1)
        items = [x.copy() for x in LOGS if x["id"] > after][:250]
        cursor = items[-1]["id"] if items else LOG_ID
    return {"logs": items, "next_cursor": cursor, "session": SESSION, "gap": gap}


def receive_line(line: str) -> None:
    match = REPLY_RE.fullmatch(line.rstrip("\r"))
    if match:
        ident, ok, response = match.groups()
        with PENDING_LOCK:
            waiter = PENDING.get(ident)
            if waiter is not None:
                waiter["ok"] = ok == "1"
                waiter["response"] = response
                waiter["event"].set()
    add_log(line)


class SerialLines:
    """Buffer bytes, not decoded chunks, and discard overlong lines to newline."""
    def __init__(self):
        self.buffer = bytearray()
        self.discard = False

    def feed(self, data: bytes) -> None:
        for byte in data:
            if byte == 10:
                if self.discard:
                    add_log("JR_PANEL_ERROR linha_serial_longa_descartada")
                elif self.buffer:
                    receive_line(self.buffer.decode("utf-8", errors="replace").rstrip("\r"))
                self.buffer.clear()
                self.discard = False
            elif not self.discard:
                if len(self.buffer) >= 4096:
                    self.buffer.clear()
                    self.discard = True
                else:
                    self.buffer.append(byte)


def close_serial(reason="Serial desconectado", expected=None) -> None:
    global SERIAL
    with SERIAL_LOCK:
        if expected is not None and SERIAL is not expected:
            return
        old, SERIAL = SERIAL, None
        if old:
            try:
                old.close()
            except Exception:
                pass
    with PENDING_LOCK:
        for waiter in PENDING.values():
            waiter.update(ok=False, response="Serial desconectado; execucao nao confirmada")
            waiter["event"].set()
    add_log(reason)


def reader_loop() -> None:
    previous = None
    lines = SerialLines()
    while not READER_STOP.is_set():
        with SERIAL_LOCK:
            current = SERIAL
        if current is not previous:
            lines = SerialLines()
            previous = current
        if not current or not current.is_open:
            READER_STOP.wait(0.1)
            continue
        try:
            data = current.read(512)
            with SERIAL_LOCK:
                same = SERIAL is current
            if data and same:
                lines.feed(data)
        except Exception:
            close_serial("JR_PANEL_ERROR leitura_serial; reconecte a porta", expected=current)
            READER_STOP.wait(0.2)


def ensure_reader() -> None:
    global READER_THREAD
    if READER_THREAD is None or not READER_THREAD.is_alive():
        READER_STOP.clear()
        READER_THREAD = threading.Thread(target=reader_loop, daemon=True)
        READER_THREAD.start()


def validate_command(command: str) -> str:
    if not command.strip() or len(command.encode("utf-8")) > MAX_COMMAND_BYTES:
        raise ValueError("Comando vazio ou maior que 768 bytes; nada enviado")
    if any(ord(ch) < 32 or ord(ch) == 127 for ch in command):
        raise ValueError("Caracteres de controle nao permitidos")
    return command


def serial_request(command: str, timeout: float | None = None) -> str:
    validate_command(command)
    if timeout is None:
        timeout = 30.0 if command.strip().lower() == "camera_test" else 5.0
    if not SEND_LOCK.acquire(timeout=1.0):
        raise RuntimeError("Outro comando esta aguardando confirmacao")
    ident = uuid.uuid4().hex[:16]
    waiter = {"event": threading.Event(), "ok": False, "response": ""}
    try:
        with PENDING_LOCK:
            PENDING[ident] = waiter
        payload = ("@" + ident + " " + command + "\n").encode("utf-8")
        with SERIAL_LOCK:
            if not SERIAL or not SERIAL.is_open:
                raise RuntimeError("Serial nao conectada")
            try:
                written = SERIAL.write(payload)
                if written != len(payload):
                    raise RuntimeError("escrita parcial")
            except Exception as exc:
                raise RuntimeError("Falha escrevendo Serial; execucao nao confirmada") from exc
        if not waiter["event"].wait(timeout):
            raise RuntimeError("Sem confirmacao do firmware. Execucao incerta; consulte status antes de repetir. Use firmware e painel revisados juntos.")
        if not waiter["ok"]:
            raise RuntimeError(sanitize_log_line(waiter["response"]))
        return waiter["response"]
    finally:
        with PENDING_LOCK:
            PENDING.pop(ident, None)
        SEND_LOCK.release()


def safe_ip(value: str) -> str:
    try:
        address = ipaddress.IPv4Address((value or "").strip())
    except ipaddress.AddressValueError as exc:
        raise ValueError("Informe somente o IPv4 local do ESP32, sem http:// ou caminho") from exc
    if not address.is_private or address.is_loopback or address.is_multicast or address.is_unspecified:
        raise ValueError("Somente IPv4 de rede local permitido")
    return str(address)


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


def wifi_request(ip: str, command: str) -> str:
    validate_command(command)
    if command.lstrip().lower().startswith("wifi_"):
        raise ValueError("Configuracao Wi-Fi somente pela Serial")
    if command.strip().lower() == "camera_test":
        raise ValueError("Teste de camera disponivel pelo painel no modo Serial USB / COM4")
    url = "http://" + safe_ip(ip) + "/cmd"
    request = urllib.request.Request(url, data=command.encode("utf-8"), method="POST", headers={"Content-Type": "text/plain; charset=utf-8", "X-JrBot-Command": "1"})
    try:
        opener = urllib.request.build_opener(urllib.request.ProxyHandler({}), NoRedirect())
        with opener.open(request, timeout=5) as reply:
            body = reply.read(4097)
            if len(body) > 4096:
                raise RuntimeError("Resposta Wi-Fi longa demais")
            text = body.decode("utf-8", errors="replace")
            if text.startswith(("JR_ERROR", "JR_WIFI_ERROR")):
                raise RuntimeError(sanitize_log_line(text))
            verb = command.strip().split(" ", 1)[0].lower()
            expected = "JR_STATUS protocol=2 " if verb == "status" else "JR_HELP protocol=2 " if verb in ("help", "ajuda") else "JR_OK "
            if not text.startswith(expected):
                raise RuntimeError("Resposta Wi-Fi fora do protocolo; execucao nao confirmada")
            if verb in ("audio_volume", "volume"):
                requested = command.strip().split(" ", 1)[-1].strip()
                applied = re.search(r"(?:^| )audio_volume=(\d+)(?: |$)", text)
                if not applied or not requested.lstrip("+").isdigit() or int(applied.group(1)) != int(requested):
                    raise RuntimeError("Volume devolvido pelo firmware diverge do solicitado")
            return text
    except urllib.error.HTTPError as exc:
        detail = sanitize_log_line(exc.read(4096).decode("utf-8", errors="replace"))
        raise RuntimeError("ESP32 recusou o comando: " + detail) from exc
    except (urllib.error.URLError, TimeoutError, OSError) as exc:
        raise RuntimeError("Falha de comunicacao Wi-Fi; execucao nao confirmada") from exc


class Handler(BaseHTTPRequestHandler):
    def setup(self):
        super().setup()
        self.connection.settimeout(10)

    def _send(self, code=200, body="", ctype="text/plain; charset=utf-8"):
        if isinstance(body, str):
            body = body.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.end_headers()
        try:
            self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError):
            pass

    def _allowed(self, write=False):
        host = self.headers.get("Host", "")
        port = self.server.server_port
        if host not in (f"127.0.0.1:{port}", f"localhost:{port}"):
            self._send(403, "Host local obrigatorio")
            return False
        origin = self.headers.get("Origin")
        if origin and origin != "http://" + host:
            self._send(403, "Origem nao permitida")
            return False
        if write and self.headers.get("X-JrBot-Panel") != "1":
            self._send(403, "Cabecalho do painel obrigatorio")
            return False
        return True

    def do_GET(self):
        if not self._allowed():
            return
        path = urllib.parse.urlsplit(self.path)
        params = urllib.parse.parse_qs(path.query)
        if path.path == "/":
            self._send(200, (ROOT / "index.html").read_text(encoding="utf-8").replace("{APP_VERSION}", APP_VERSION), "text/html; charset=utf-8")
        elif path.path == "/panel.js":
            self._send(200, (ROOT / "panel.js").read_text(encoding="utf-8"), "text/javascript; charset=utf-8")
        elif path.path == "/ports":
            try:
                ports = [] if list_ports is None else [p.device for p in list_ports.comports()]
                self._send(200, json.dumps({"ports": ports}), "application/json")
            except Exception:
                self._send(500, "Falha enumerando portas")
        elif path.path == "/logs":
            try:
                after = max(0, int(params.get("after", ["0"])[0]))
            except ValueError:
                self._send(400, "Cursor invalido")
                return
            page = log_page(after, params.get("session", [""])[0])
            with SERIAL_LOCK:
                page["serial_connected"] = bool(SERIAL and SERIAL.is_open)
                page["serial_port"] = getattr(SERIAL, "port", "") if page["serial_connected"] else ""
            self._send(200, json.dumps(page), "application/json")
        elif path.path.startswith("/camera/"):
            self._send(503, "Camera desabilitada nesta candidata")
        else:
            self._send(404, "Rota nao encontrada")

    def do_POST(self):
        global SERIAL
        if not self._allowed(write=True):
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if length < 0 or length > 8192 or self.headers.get("Transfer-Encoding"):
                self.close_connection = True
                self._send(413, "Corpo muito grande ou formato nao permitido")
                return
            raw = self.rfile.read(length)
            if len(raw) != length:
                raise ValueError("Corpo incompleto")
            data = urllib.parse.parse_qs(raw.decode("utf-8"), keep_blank_values=True, max_num_fields=12, errors="strict")
            get = lambda key, default="": data.get(key, [default])[0]
            if self.path == "/connect":
                if serial is None:
                    raise RuntimeError("pyserial nao instalado; instale requirements.txt")
                port = get("port").strip()
                available = {p.device for p in list_ports.comports()}
                if port not in available:
                    raise ValueError("Selecione uma porta Serial detectada")
                with SEND_LOCK:
                    close_serial()
                    opened = None
                    try:
                        # Prepare control lines before open; do not intentionally reset.
                        # OS/USB drivers may still produce a brief DTR/RTS transition.
                        opened = serial.Serial(port=None, baudrate=BAUD, timeout=0.1, write_timeout=2)
                        opened.dtr = False
                        opened.rts = False
                        opened.port = port
                        opened.open()
                    except Exception as exc:
                        if opened is not None:
                            opened.close()
                        raise RuntimeError("Nao foi possivel abrir a porta. Feche o monitor e outros paineis; selecione COM4 nesta montagem.") from exc
                    with SERIAL_LOCK:
                        SERIAL = opened
                    ensure_reader()
                self._send(200, "Serial conectado " + port)
            elif self.path == "/disconnect":
                with SEND_LOCK:
                    close_serial()
                self._send(200, "Serial desconectado")
            elif self.path == "/send":
                command = validate_command(get("command"))
                mode = get("mode", "serial")
                if mode == "wifi":
                    result = wifi_request(get("ip"), command)
                elif mode == "serial":
                    result = serial_request(command)
                else:
                    raise ValueError("Modo invalido")
                if mode == "wifi":
                    add_log(result)
                self._send(200, result)
            else:
                self._send(404, "Rota nao encontrada")
        except (ValueError, UnicodeError) as exc:
            self._send(400, sanitize_log_line(str(exc)))
        except RuntimeError as exc:
            self._send(502, sanitize_log_line(str(exc)))
        except (TimeoutError, OSError):
            self.close_connection = True
            self._send(503, "Falha de I/O; operacao nao confirmada")

    def log_message(self, fmt, *args):
        return


if __name__ == "__main__":
    import sys
    import webbrowser
    try:
        server = ThreadingHTTPServer(("127.0.0.1", 8765), Handler)
    except OSError:
        print("Porta 8765 ocupada. Feche o painel anterior antes de abrir esta versao.")
        raise SystemExit(1)
    print(APP_VERSION + " - http://127.0.0.1:8765 - painel COM4 / flash COM6")
    if "--browser" in sys.argv:
        webbrowser.open("http://127.0.0.1:8765")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        READER_STOP.set()
        close_serial()
        server.server_close()
