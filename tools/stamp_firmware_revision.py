#!/usr/bin/env python3
"""Stamp the checked-out Git commit in a gitignored C header before every build.

The firmware reports this ID through Serial boot, version and status endpoints.
This does not require network access; INSTALAR.bat has already synced origin.
"""
from __future__ import annotations

from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "firmware" / "core" / "jr_firmware_revision_local.h"


def stamp(root: Path = ROOT) -> str:
    sha = subprocess.check_output(
        ["git", "-C", str(root), "rev-parse", "--verify", "HEAD"],
        text=True, encoding="ascii", stderr=subprocess.PIPE,
    ).strip().lower()
    if not re.fullmatch(r"[a-f0-9]{40}", sha):
        raise ValueError("Git HEAD invalido: nao foi possivel gerar identidade de build")
    short = sha[:8]
    header = root / "firmware" / "core" / "jr_firmware_revision_local.h"
    header.parent.mkdir(parents=True, exist_ok=True)
    payload = (
        "/* Auto-generated locally by stamp_firmware_revision.py. Do not commit. */\n"
        f'#define JR_FIRMWARE_GIT_COMMIT "{short}"\n'
    )
    if not header.exists() or header.read_text(encoding="ascii") != payload:
        header.write_text(payload, encoding="ascii", newline="\n")
        print("[BUILD] Git commit atualizado no firmware:", short)
    else:
        print("[BUILD] Git commit ja identificado no firmware:", short)
    return short


if __name__ == "__main__":
    try:
        stamp()
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        print("[ERRO] Nao foi possivel identificar o commit do firmware:", exc, file=sys.stderr)
        raise SystemExit(1)
