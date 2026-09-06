#!/usr/bin/env python3
"""JrBot panel: transporte serial simples, uma linha por comando, inspirado na V1."""
from __future__ import annotations

import threading
import uuid
import webbrowser

import app


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


app.serial_request = stable_serial_request
app.close_serial = quiet_close_serial
app.APP_VERSION = "JRBOT-PANEL-V2-06-DYNAMIC-PORTS"


def main() -> int:
    try:
        server = app.ThreadingHTTPServer(("127.0.0.1", 8765), app.Handler)
    except OSError:
        print("Porta 8765 ocupada. Feche o painel anterior antes de abrir esta versao.")
        return 1
    print(app.APP_VERSION + " - http://127.0.0.1:8765 - selecione qualquer porta COM detectada")
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
