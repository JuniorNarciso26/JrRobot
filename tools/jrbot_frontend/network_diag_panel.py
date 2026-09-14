#!/usr/bin/env python3
"""Acrescenta o diagnostico detalhado de rede ao JavaScript principal do painel."""
from __future__ import annotations

import urllib.parse

import app

_PREVIOUS_DO_GET = app.Handler.do_GET


def network_diag_do_get(self) -> None:
    parsed = urllib.parse.urlsplit(self.path)
    if parsed.path == "/panel.js":
        if not self._allowed():
            return
        base = (app.ROOT / "panel.js").read_text(encoding="utf-8")
        diag = (app.ROOT / "wifi_diag.js").read_text(encoding="utf-8")
        self._send(200, base + "\n\n" + diag, "text/javascript; charset=utf-8")
        return
    _PREVIOUS_DO_GET(self)


app.Handler.do_GET = network_diag_do_get
