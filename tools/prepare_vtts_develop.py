#!/usr/bin/env python3
"""Prepare pinned Apache-2.0 vTTS runtime, pt-BR models and lexicon for develop.

The main branch never uses these files. Vendor code, binary voice models and
build artifacts are gitignored, so accidental binary commits are prevented.

Reuse the working JrBot_VTTS_Test lab if present, otherwise clone the same
pinned upstream source and download the two official v0.11.0 model releases.
"""
from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess
import sys
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
VENDOR = ROOT / "firmware" / "vendor"
SRC = VENDOR / "vtts_src"
ASSETS = VENDOR / "vtts_assets"
UPSTREAM_REPO = "https://github.com/tuanpmt/vtts.git"
UPSTREAM_SHA = "010cd22fd51f58dfa2414824cb8d6345e70cafc6"
RELEASE = "https://github.com/tuanpmt/vtts/releases/download/v0.11.0/"
MODELS = {
    "model_s.vtts": ("vtts-pten-nupt-s-v0.11.q8.vtts", 1222368),
    "model_m.vtts": ("vtts-pten-nupt-m-v0.11.q8.vtts", 2607840),
}


def run(args: list[str], cwd: Path | None = None, env: dict[str, str] | None = None) -> None:
    print("[CMD]", " ".join(map(str, args)), flush=True)
    subprocess.run(args, check=True, cwd=str(cwd) if cwd else None, env=env)


def revision(checkout: Path) -> str:
    if not (checkout / ".git").exists():
        return ""
    return subprocess.check_output(
        ["git", "-C", str(checkout), "rev-parse", "HEAD"], text=True,
    ).strip()


def copy_model(source: Path, dest: Path, expected: int) -> bool:
    if not source.is_file() or source.stat().st_size != expected:
        return False
    if not dest.is_file() or dest.stat().st_size != expected:
        shutil.copy2(source, dest)
        print("[REUSE]", dest)
    return True


def find_lab() -> Path | None:
    hints = [
        ROOT.parent / "JrBot_VTTS_Test",
        Path.home() / "Desktop" / "JrBot_VTTS_Test",
        Path.home() / "OneDrive" / "Desktop" / "JrBot_VTTS_Test",
        ROOT,
    ]
    for base in hints:
        path = base / "experiments" / "vtts-hw04" / ".work" / "vtts"
        if revision(path) == UPSTREAM_SHA:
            print("[OK] Encontrado teste de voz vTTS aprovado:", path)
            return path
    return None


def ensure_upstream() -> Path:
    lab = find_lab()
    if lab:
        return lab
    checkout = VENDOR / ".upstream"
    if not (checkout / ".git").exists():
        if checkout.exists():
            raise RuntimeError(f"{checkout} ja existe sem .git; remova manualmente se nao for seu arquivo.")
        run(["git", "clone", "--no-checkout", "--depth", "1", UPSTREAM_REPO, str(checkout)])
        run(["git", "-C", str(checkout), "fetch", "--depth", "1", "origin", UPSTREAM_SHA])
        run(["git", "-C", str(checkout), "checkout", "--detach", UPSTREAM_SHA])
    if revision(checkout) != UPSTREAM_SHA:
        raise RuntimeError("Versao do upstream vTTS nao corresponde ao commit pinado; nao sobrescrito.")
    return checkout


def download_model(filename: str, expected: int, dest: Path) -> None:
    temp = dest.with_suffix(dest.suffix + ".partial")
    if temp.exists():
        temp.unlink()
    print("[DOWNLOAD] vTTS pt-BR:", filename, flush=True)
    req = urllib.request.Request(RELEASE + filename, headers={"User-Agent": "JrBot-develop-vTTS"})
    try:
        with urllib.request.urlopen(req, timeout=120) as inp, temp.open("wb") as out:
            shutil.copyfileobj(inp, out)
        if temp.stat().st_size != expected:
            raise RuntimeError(f"{filename}: tamanho incorreto {temp.stat().st_size}, esperado {expected}")
        temp.replace(dest)
    finally:
        if temp.exists():
            temp.unlink()


def prepare() -> None:
    VENDOR.mkdir(parents=True, exist_ok=True)
    ASSETS.mkdir(parents=True, exist_ok=True)
    upstream = ensure_upstream()
    if revision(upstream) != UPSTREAM_SHA:
        raise RuntimeError("Source vTTS nao confere com a versao validada.")

    marker = SRC / ".vtts_revision"
    if marker.exists() and marker.read_text(encoding="utf-8").strip() != UPSTREAM_SHA:
        raise RuntimeError("Source local tem outra revisao; nao sera sobrescrito.")
    if not marker.exists():
        SRC.mkdir(parents=True, exist_ok=True)
        shutil.copytree(upstream / "runtime", SRC / "runtime", dirs_exist_ok=True)
        for name in ("LICENSE", "NOTICE"):
            shutil.copy2(upstream / name, SRC / name)
        marker.write_text(UPSTREAM_SHA + "\n", encoding="utf-8")
        print("[OK] Runtime vTTS importado:", SRC)

    lab_models = upstream / "firmware" / "esp32" / "main"
    for dest_name, (filename, expected) in MODELS.items():
        dest = ASSETS / dest_name
        if not copy_model(dest, dest, expected):
            if not copy_model(lab_models / dest_name, dest, expected):
                download_model(filename, expected, dest)
        if dest.stat().st_size != expected:
            raise RuntimeError(f"Modelo invalido: {dest}")

    lex = ASSETS / "lex_en.bin"
    if not lex.is_file() or lex.stat().st_size < 500000:
        if (lab_models / "lex_en.bin").is_file() and (lab_models / "lex_en.bin").stat().st_size >= 500000:
            shutil.copy2(lab_models / "lex_en.bin", lex)
            print("[REUSE] Dicionario ingles:", lex)
        else:
            env = os.environ.copy()
            env["PYTHONPATH"] = str(upstream / "src") + os.pathsep + env.get("PYTHONPATH", "")
            print("[INFO] Gerando lex_en.bin via vTTS upstream, pode usar CMUdict.")
            run([sys.executable, "scripts/frontend/build_lexicon.py", "--out", str(lex)],
                cwd=upstream, env=env)
    if not lex.is_file() or lex.stat().st_size < 500000:
        raise RuntimeError("Dicionario lex_en.bin ausente ou incompleto.")

    print("[OK] vTTS preparado no develop; fontes Apache-2.0 + NOTICE:", UPSTREAM_SHA)
    print("[OK] Models: S", (ASSETS / "model_s.vtts").stat().st_size,
          "M", (ASSETS / "model_m.vtts").stat().st_size,
          "Lex", lex.stat().st_size)
    print("[INFO] Nenhum firmware foi gravado. Execute INSTALAR.bat build para verificar o tamanho.")


if __name__ == "__main__":
    try:
        prepare()
    except (OSError, RuntimeError, subprocess.CalledProcessError, ValueError) as exc:
        print("[ERRO]", exc, file=sys.stderr)
        sys.exit(1)
