#!/usr/bin/env python3
"""Inicializador do painel de desenvolvimento com encerramento simples por ENTER."""
from __future__ import annotations

import threading
import webbrowser

import app
import docs_panel  # noqa: F401 - aplica extensoes/rotas do painel


def _wait_for_enter(server: app.ThreadingHTTPServer) -> None:
    try:
        input("\nPainel em execucao. Pressione ENTER para encerrar...\n")
    except EOFError:
        return
    server.shutdown()


def main() -> int:
    try:
        server = app.ThreadingHTTPServer(("127.0.0.1", 8765), app.Handler)
    except OSError:
        print("Porta 8765 ocupada. Feche o painel anterior antes de abrir esta versao.")
        return 1

    print(app.APP_VERSION + " - http://127.0.0.1:8765")
    app.ensure_reader()
    webbrowser.open("http://127.0.0.1:8765")

    stop_thread = threading.Thread(target=_wait_for_enter, args=(server,), daemon=True)
    stop_thread.start()

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
