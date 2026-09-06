#!/usr/bin/env python3
"""JrBot panel: serial simples + diagnostico Wi-Fi + foto JPEG da OV5640."""
from __future__ import annotations

import threading
import urllib.error
import urllib.parse
import urllib.request
import uuid
import webbrowser

import app

MAX_JPEG_BYTES = 512 * 1024


def stable_serial_request(command: str, timeout: float | None = None) -> str:
    app.validate_command(command)
    verb = command.strip().lower().split(" ", 1)[0]
    if timeout is None:
        timeout = 30.0 if verb == "camera_test" else 8.0
    if not app.SEND_LOCK.acquire(timeout=2.0):
        raise RuntimeError("Outro comando esta aguardando confirmacao")

    ident = uuid.uuid4().hex[:16]
    waiter = {"event": threading.Event(), "ok": False, "response": ""}
    payload = ("@" + ident + " " + command.strip() + "\n").encode("ascii", errors="strict")

    try:
        with app.PENDING_LOCK:
            app.PENDING[ident] = waiter
        with app.SERIAL_LOCK:
            if not app.SERIAL or not app.SERIAL.is_open:
                raise RuntimeError("Serial nao conectada")
            try:
                written = app.SERIAL.write(payload)
                app.SERIAL.flush()
                if written != len(payload):
                    raise RuntimeError("escrita parcial")
            except Exception as exc:
                raise RuntimeError("Falha escrevendo Serial; execucao nao confirmada") from exc

        if not waiter["event"].wait(timeout):
            app.close_serial("JR_PANEL_WARN timeout; porta serial fechada automaticamente")
            raise RuntimeError("Sem confirmacao do firmware. A porta foi fechada automaticamente.")
        if not waiter["ok"]:
            raise RuntimeError(app.sanitize_log_line(waiter["response"]))
        return waiter["response"]
    finally:
        with app.PENDING_LOCK:
            app.PENDING.pop(ident, None)
        app.SEND_LOCK.release()


_original_close_serial = app.close_serial


def quiet_close_serial(reason="Serial desconectado", expected=None) -> None:
    with app.SERIAL_LOCK:
        had_serial = bool(app.SERIAL)
    if not had_serial and expected is None and reason == "Serial desconectado":
        reason = ""
    _original_close_serial(reason, expected)


def wifi_capture(ip: str) -> tuple[bytes, str]:
    safe = app.safe_ip(ip)
    request = urllib.request.Request(
        "http://" + safe + "/capture",
        method="GET",
        headers={"Accept": "image/jpeg", "Cache-Control": "no-cache"},
    )
    try:
        opener = urllib.request.build_opener(urllib.request.ProxyHandler({}), app.NoRedirect())
        with opener.open(request, timeout=12) as reply:
            ctype = reply.headers.get_content_type()
            body = reply.read(MAX_JPEG_BYTES + 1)
            if len(body) > MAX_JPEG_BYTES:
                raise RuntimeError("Foto da camera excedeu o limite de 512 KiB")
            if ctype != "image/jpeg" or len(body) < 4 or body[:2] != b"\xff\xd8":
                raise RuntimeError("ESP32 nao devolveu um JPEG valido")
            app.add_log(f"JR_CAMERA_WIFI photo=received ip={safe} bytes={len(body)}")
            return body, safe
    except urllib.error.HTTPError as exc:
        detail = app.sanitize_log_line(exc.read(4096).decode("utf-8", errors="replace"))
        raise RuntimeError("ESP32 recusou a foto: " + detail) from exc
    except (urllib.error.URLError, TimeoutError, OSError) as exc:
        raise RuntimeError("Falha buscando foto da camera pelo Wi-Fi") from exc


_original_do_get = app.Handler.do_GET


def enhanced_do_get(self) -> None:
    parsed = urllib.parse.urlsplit(self.path)
    if parsed.path == "/":
        if not self._allowed():
            return
        html = (app.ROOT / "index.html").read_text(encoding="utf-8").replace("{APP_VERSION}", app.APP_VERSION)
        html = html.replace("</body>", '<script src="/photo_panel.js"></script></body>')
        self._send(200, html, "text/html; charset=utf-8")
        return
    if parsed.path == "/photo_panel.js":
        if not self._allowed():
            return
        self._send(200, (app.ROOT / "photo_panel.js").read_text(encoding="utf-8"), "text/javascript; charset=utf-8")
        return
    if parsed.path == "/camera/capture":
        if not self._allowed():
            return
        params = urllib.parse.parse_qs(parsed.query)
        try:
            body, safe = wifi_capture(params.get("ip", [""])[0])
            self._send(200, body, "image/jpeg")
            app.add_log(f"JR_CAMERA_PANEL photo=delivered ip={safe} bytes={len(body)}")
        except ValueError as exc:
            self._send(400, app.sanitize_log_line(str(exc)))
        except RuntimeError as exc:
            self._send(502, app.sanitize_log_line(str(exc)))
        return
    _original_do_get(self)


app.serial_request = stable_serial_request
app.close_serial = quiet_close_serial
app.Handler.do_GET = enhanced_do_get
app.APP_VERSION = "JRBOT-PANEL-V2-08-CAMERA-PHOTO"


def main() -> int:
    try:
        server = app.ThreadingHTTPServer(("127.0.0.1", 8765), app.Handler)
    except OSError:
        print("Porta 8765 ocupada. Feche o painel anterior antes de abrir esta versao.")
        return 1
    print(app.APP_VERSION + " - http://127.0.0.1:8765 - portas COM dinamicas + foto Wi-Fi")
    app.ensure_reader()
    webbrowser.open("http://127.0.0.1:8765")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        app.READER_STOP.set()
        app.close_serial()
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
