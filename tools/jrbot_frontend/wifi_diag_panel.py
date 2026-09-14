#!/usr/bin/env python3
"""Injeta o diagnostico Wi-Fi no painel sem duplicar o fluxo de provisionamento."""
from __future__ import annotations

import urllib.parse

import app
import wifi_panel

_PREVIOUS_DO_GET = app.Handler.do_GET

# O wifi_panel monta a pagina principal em tempo de requisicao, entao podemos
# acrescentar o script de diagnostico sem copiar o formulario de configuracao.
wifi_panel._WIFI_SCRIPT += '<script src="/wifi_diag.js"></script>'


def wifi_diag_do_get(self) -> None:
    parsed = urllib.parse.urlsplit(self.path)
    if parsed.path == "/wifi_diag.js":
        if not self._allowed():
            return
        script = (app.ROOT / "wifi_diag.js").read_text(encoding="utf-8")
        self._send(200, script, "application/javascript; charset=utf-8")
        return
    _PREVIOUS_DO_GET(self)


app.Handler.do_GET = wifi_diag_do_get
app.APP_VERSION = "JRBOT-PANEL-V2-15-WIFI-DIAG"
