#!/usr/bin/env python3
"""JrBot V2: local panel, bounded requests and correlated Serial replies."""
from __future__ import annotations
import ipaddress
import json
import re
import threading
import time
import uuid
import solana_skill
import wallet_auth
import wallet_network
import wallet_purchase
import wallet_skills
import wallet_execution
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
APP_VERSION = "JRBOT-PANEL-V1S-SKILLS-14"
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

    def _send(self, code=200, body="", ctype="text/plain; charset=utf-8", cookie=None):
        if isinstance(body, str):
            body = body.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        if cookie:
            self.send_header("Set-Cookie", cookie)
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
        elif path.path == "/wallet_panel.js":
            self._send(200, (ROOT / "wallet_panel.js").read_text(encoding="utf-8"), "text/javascript; charset=utf-8")
        elif path.path == "/jrskill/wallet/status":
            token = wallet_auth.cookie_token(self.headers.get("Cookie"))
            status = wallet_auth.STORE.status(token, "http://" + self.headers["Host"])
            self._send(200, json.dumps(status), "application/json; charset=utf-8")
        elif path.path == "/jrskill/wallet/skills":
            token = wallet_auth.cookie_token(self.headers.get("Cookie"))
            origin = "http://" + self.headers["Host"]
            status = wallet_auth.STORE.status(token, origin)
            if not status.get("authenticated"):
                self._send(401, "Autentique a carteira para buscar suas Skills")
                return
            try:
                result = wallet_skills.discover(status["address"])
                after = wallet_auth.STORE.status(token, origin)
                if not after.get("authenticated") or after.get("address") != status["address"]:
                    self._send(401, "Sessao alterada ou expirada durante a busca")
                    return
            except (ValueError, TimeoutError, OSError) as exc:
                detail = solana_skill.rpc_error_detail(exc)
                add_log("JR_SKILL_DISCOVERY result=error buyer=" + status["address"] + " detail=" + detail)
                self._send(502, "Busca de Skills nao confirmada: " + detail)
                return
            add_log("JR_SKILL_DISCOVERY result=ok buyer=" + status["address"] + " count=" + str(len(result["skills"])) + " rpc_slot=" + str(result["rpc_slot"]))
            self._send(200, json.dumps(result), "application/json; charset=utf-8")
        elif path.path == "/jrskill/wallet/purchase/status":
            token = wallet_auth.cookie_token(self.headers.get("Cookie"))
            origin = "http://" + self.headers["Host"]
            status = wallet_auth.STORE.status(token, origin)
            if not status.get("authenticated"):
                self._send(401, "Autentique a carteira para consultar sua licenca")
                return
            try:
                result = wallet_purchase.inspect(status["address"])
                wallet_purchase.STORE.authorized(token, origin, status["address"])
            except (ValueError, TimeoutError, OSError) as exc:
                self._send(502, "Licenca nao confirmada: " + str(exc))
                return
            self._send(200, json.dumps(result), "application/json; charset=utf-8")
        elif path.path == "/jrskill/wallet/devnet":
            token = wallet_auth.cookie_token(self.headers.get("Cookie"))
            origin = "http://" + self.headers["Host"]
            status = wallet_auth.STORE.status(token, origin)
            if not status.get("authenticated"):
                self._send(401, "Autentique a carteira antes de consultar Devnet e saldo")
                return
            try:
                result = wallet_network.inspect(status["address"])
                after = wallet_auth.STORE.status(token, origin)
                if not after.get("authenticated") or after.get("address") != status["address"]:
                    self._send(401, "Sessao alterada ou expirada durante a consulta")
                    return
            except (ValueError, TimeoutError, OSError) as exc:
                self._send(502, "Consulta Devnet nao confirmada: " + str(exc))
                return
            self._send(200, json.dumps(result), "application/json; charset=utf-8")
        elif path.path == "/jrskill/api-sequence":
            sequence = ROOT / "api_sequences" / "jrskill_runtime_api_baseline_01.json"
            self._send(200, sequence.read_text(encoding="utf-8"), "application/json; charset=utf-8")
        elif path.path == "/jrskill/devnet-status":
            try:
                status = solana_skill.check_connection()
            except Exception:
                status = {"available": False, "cluster": "devnet"}
            self._send(200, json.dumps(status), "application/json; charset=utf-8")
        elif path.path == "/jrskill/skill-devnet":
            try:
                proof = solana_skill.load_skill()
            except Exception as exc:
                add_log("JR_SKILL_SOLANA result=error detail=" + str(exc))
                self._send(502, "Leitura Devnet recusada: " + str(exc))
                return
            self._send(200, json.dumps(proof), "application/json; charset=utf-8")
        elif path.path == "/jrskill/skill":
            skill = ROOT / "jrskill" / "skills" / "minimal_recipe_01.json"
            self._send(200, skill.read_text(encoding="utf-8"), "application/json; charset=utf-8")
        elif path.path.startswith("/jrskill/recipe/"):
            name = path.path[len("/jrskill/recipe/"):]
            if not re.fullmatch(r"[A-Za-z0-9_-]{1,64}", name):
                self._send(400, "Nome de recipe invalido")
                return
            recipe = ROOT / "jrskill" / "recipes" / (name + ".json")
            if not recipe.is_file():
                self._send(404, "Recipe nao encontrada")
                return
            self._send(200, recipe.read_text(encoding="utf-8"), "application/json; charset=utf-8")
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
            if self.path.startswith("/jrskill/wallet/"):
                origin = "http://" + self.headers["Host"]
                if self.headers.get("Origin") != origin:
                    self._send(403, "Origem local obrigatoria para autenticacao")
                    return
                token = wallet_auth.cookie_token(self.headers.get("Cookie"))
                cookie = None
                if self.path == "/jrskill/wallet/challenge":
                    token, result = wallet_auth.STORE.challenge(token, origin, get("address"))
                    cookie = f"{wallet_auth.COOKIE}={token}; HttpOnly; SameSite=Strict; Path=/jrskill/wallet; Max-Age={wallet_auth.SESSION_TTL}"
                elif self.path == "/jrskill/wallet/verify":
                    result = wallet_auth.STORE.verify(token, origin, get("id"), get("signature"))
                    cookie = f"{wallet_auth.COOKIE}={token}; HttpOnly; SameSite=Strict; Path=/jrskill/wallet; Max-Age={wallet_auth.SESSION_TTL}"
                    add_log("JR_WALLET_AUTH state=authenticated address=" + result["address"])
                elif self.path == "/jrskill/wallet/logout":
                    wallet_auth.STORE.logout(token)
                    result = {"authenticated": False}
                    cookie = f"{wallet_auth.COOKIE}=; HttpOnly; SameSite=Strict; Path=/jrskill/wallet; Max-Age=0"
                elif self.path in ('/jrskill/wallet/execution/start', '/jrskill/wallet/execution/send'):
                    status = wallet_auth.STORE.status(token, origin)
                    if not status.get('authenticated'):
                        add_log('JR_SKILL_AUTHORIZATION result=blocked reason=authentication_required')
                        self._send(401, 'Autentique a carteira para executar a Skill da Devnet')
                        return
                    try:
                        if self.path.endswith('/start'):
                            result = wallet_execution.STORE.start(token, origin)
                            add_log('JR_SKILL_AUTHORIZATION result=allowed buyer=' + result['buyer'] + ' license=' + result['license'])
                        else:
                            command = validate_command(get('command'))
                            result = {'reply': wallet_execution.STORE.send(token, origin, get('permit'), command, serial_request)}
                            add_log('JR_SKILL_AUTHORIZATION result=command_sent buyer=' + status['address'])
                    except (ValueError, RuntimeError, TimeoutError, OSError) as exc:
                        add_log('JR_SKILL_AUTHORIZATION result=blocked buyer=' + status['address'] + ' detail=' + solana_skill.rpc_error_detail(str(exc)))
                        raise
                elif self.path in ("/jrskill/wallet/purchase/quote", "/jrskill/wallet/purchase/submit"):
                    status = wallet_auth.STORE.status(token, origin)
                    if not status.get("authenticated"):
                        self._send(401, "Autentique a carteira antes da compra")
                        return
                    if self.path.endswith("/quote"):
                        result = wallet_purchase.STORE.quote(token, origin, status["address"])
                    else:
                        try:
                            result = wallet_purchase.STORE.submit(token, origin, status["address"], get("quote_id"), get("signed_transaction"))
                        except (ValueError, TimeoutError, OSError) as exc:
                            if isinstance(exc, wallet_purchase.PurchaseRejected):
                                diagnostic = dict(exc.diagnostics)
                                comparisons = {key: diagnostic.pop(key) for key in ('expected', 'actual') if key in diagnostic}
                                add_log('JR_SKILL_PURCHASE_DIAG code=' + exc.code + ' metadata=' + json.dumps(diagnostic, separators=(',', ':')))
                                for side, summary in comparisons.items():
                                    summary = dict(summary)
                                    instructions = summary.pop('instructions')
                                    add_log('JR_SKILL_PURCHASE_DIAG side=' + side + ' message=' + json.dumps(summary, separators=(',', ':')))
                                    for index, item in enumerate(instructions):
                                        add_log('JR_SKILL_PURCHASE_DIAG side=' + side + ' instruction=' + str(index) + ' detail=' + json.dumps(item, separators=(',', ':')))
                            add_log("JR_SKILL_PURCHASE stage=server_validation state=rejected buyer=" + status["address"]
                                    + " relay_not_completed=true detail=" + solana_skill.rpc_error_detail(type(exc).__name__ + ': ' + str(exc)))
                            raise
                    add_log("JR_SKILL_PURCHASE buyer=" + status["address"] + " state=" + result.get("state", "quote")
                             + " signature=" + result.get("signature", "none"))
                    if result.get("error_detail"):
                        add_log("JR_SKILL_PURCHASE stage=rpc_relay state=unknown buyer=" + status["address"]
                                + " signature=" + result.get("signature", "none") + " detail=" + result["error_detail"])
                    if 'compute_units' in result:
                        add_log('JR_SKILL_PURCHASE_BUDGET ' + json.dumps({key: result[key] for key in
                            ('compute_units', 'unit_price_micro_lamports', 'priority_fee_lamports', 'fee_lamports', 'fee_limit_lamports')}, separators=(',', ':')))
                else:
                    self._send(404, "Rota nao encontrada")
                    return
                self._send(200, json.dumps(result), "application/json; charset=utf-8", cookie=cookie)
            elif self.path == "/connect":
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
