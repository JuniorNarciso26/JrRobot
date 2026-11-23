#!/usr/bin/env python3
"""Isolated vTTS nu-pt / JrBot HW04 test. Never modifies JrBot production firmware.

Run on Windows from an ESP-IDF 6.1 activated terminal:
    python experiments/vtts-hw04/run.py prepare|build|flash|monitor|all [COMx]

All external source, model weights and build artifacts are under .work/ (gitignored).
The pinned vTTS upstream and its Apache-2.0 license are kept intact in .work/vtts.
"""
from __future__ import annotations

import argparse
import filecmp
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import urllib.request

BASE = Path(__file__).resolve().parent
WORK = BASE / ".work"
UPSTREAM = WORK / "vtts"
FW = UPSTREAM / "firmware" / "esp32"
BOARD = "jrbot-hw04"
UPSTREAM_URL = "https://github.com/tuanpmt/vtts.git"
UPSTREAM_SHA = "010cd22fd51f58dfa2414824cb8d6345e70cafc6"
RELEASE = "https://github.com/tuanpmt/vtts/releases/download/v0.11.0/"
MODELS = {
    "s": ("vtts-pten-nupt-s-v0.11.q8.vtts", 1222368),
    "m": ("vtts-pten-nupt-m-v0.11.q8.vtts", 2607840),
}


def fail(message: str) -> None:
    raise RuntimeError(message)


def execute(args: list[str], cwd: Path | None = None, env: dict[str, str] | None = None) -> None:
    print("[CMD]", " ".join(map(str, args)), flush=True)
    subprocess.run(args, cwd=str(cwd) if cwd else None, env=env, check=True)


def same_file(left: Path, right: Path) -> bool:
    return left.is_file() and right.is_file() and filecmp.cmp(left, right, shallow=False)


def copy_if_changed(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    if not same_file(src, dst):
        shutil.copy2(src, dst)
        print("[OK]", dst)


def ensure_upstream() -> None:
    if not shutil.which("git"):
        fail("Git nao encontrado no PATH.")
    WORK.mkdir(parents=True, exist_ok=True)
    if not (UPSTREAM / ".git").is_dir():
        if UPSTREAM.exists():
            fail(f"{UPSTREAM} ja existe mas nao e um checkout Git.")
        execute(["git", "clone", "--no-checkout", "--depth", "1", UPSTREAM_URL, str(UPSTREAM)])
        execute(["git", "-C", str(UPSTREAM), "fetch", "--depth", "1", "origin", UPSTREAM_SHA])
        execute(["git", "-C", str(UPSTREAM), "checkout", "--detach", UPSTREAM_SHA])
    head = subprocess.check_output(
        ["git", "-C", str(UPSTREAM), "rev-parse", "HEAD"], text=True
    ).strip()
    if head != UPSTREAM_SHA:
        fail("A copia do upstream nao esta no commit fixado. Verifique .work/vtts; nao sera sobrescrita.")
    print("[OK] vTTS upstream fixado:", head)


def download_model(name: str, expected: int) -> Path:
    dest = WORK / "models" / name
    dest.parent.mkdir(parents=True, exist_ok=True)
    if dest.is_file() and dest.stat().st_size == expected:
        print("[OK] Modelo ja baixado:", name)
        return dest
    tmp = dest.with_name(dest.name + ".partial")
    if tmp.exists():
        tmp.unlink()
    req = urllib.request.Request(RELEASE + name, headers={"User-Agent": "JrBot-vTTS-HW04-lab"})
    print("[DOWNLOAD]", name, flush=True)
    try:
        with urllib.request.urlopen(req, timeout=120) as response, tmp.open("wb") as output:
            shutil.copyfileobj(response, output)
        size = tmp.stat().st_size
        if size != expected:
            fail(f"Modelo {name} tem {size} bytes; esperado {expected}. Download invalido.")
        tmp.replace(dest)
    finally:
        if tmp.exists():
            tmp.unlink()
    print("[OK] Modelo:", dest)
    return dest


def prepare() -> None:
    ensure_upstream()
    board = FW / "components" / "bsp" / "boards" / BOARD
    for name in ("board.h", "board.cmake", "sdkconfig.defaults"):
        copy_if_changed(BASE / "board" / name, board / name)
    for short, (name, size) in MODELS.items():
        source = download_model(name, size)
        copy_if_changed(source, FW / "main" / f"model_{short}.vtts")
    lexicon = FW / "main" / "lex_en.bin"
    if not lexicon.is_file() or lexicon.stat().st_size < 500000:
        print("[INFO] Gerando dicionario ingles do vTTS (necessario para o modelo pt+en).")
        print("[INFO] Na primeira execucao, o upstream baixa CMUdict e pode treinar a tabela LTS.")
        env = os.environ.copy()
        env["PYTHONPATH"] = str(UPSTREAM / "src") + os.pathsep + env.get("PYTHONPATH", "")
        execute(
            [sys.executable, "scripts/frontend/build_lexicon.py", "--out", str(lexicon)],
            cwd=UPSTREAM,
            env=env,
        )
    if not lexicon.is_file() or lexicon.stat().st_size < 500000:
        fail("Nao foi gerado um lex_en.bin valido.")
    print("[OK] Ambiente preparado. Firmware principal JrBot nao foi modificado.")


def idf_script() -> Path:
    env = os.environ.get("IDF_PATH", "").strip()
    if not env:
        fail("ESP-IDF nao ativado. Abra o terminal IDF_PowerShell do ESP-IDF 6.1 e execute novamente.")
    script = Path(env) / "tools" / "idf.py"
    if not script.is_file():
        fail(f"idf.py nao encontrado em {script}")
    version = subprocess.run(
        [sys.executable, str(script), "--version"], capture_output=True, text=True
    )
    output = (version.stdout + version.stderr).strip()
    if version.returncode or not re.search(r"\bv?6\.1(?:\.\d+)?\b", output):
        fail(f"O vTTS requer ESP-IDF 6.1 (separado da 5.5.x do JrBot). Atual: {output}")
    print("[OK]", output)
    return script


def idf_args() -> list[str]:
    return [
        "-B", f"build_{BOARD}",
        "-D", f"SDKCONFIG=sdkconfig.{BOARD}",
        "-D", "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.defaults.esp32s3;"
              f"components/bsp/boards/{BOARD}/sdkconfig.defaults",
        "-D", f"VTTS_BOARD={BOARD}",
        "-D", "VTTS_LANGS=pt",
    ]


def ensure_uart_console_config() -> bool:
    """Migrate an existing local vTTS sdkconfig from USB-JTAG to CH343/UART0.

    ESP-IDF sdkconfig overrides sdkconfig.defaults. A user who compiled the
    first revision already has the wrong console selected in sdkconfig.jrbot-hw04.
    Preserve that config before modifying it; never touch JrBot's own sdkconfig.
    """
    cfg = FW / f"sdkconfig.{BOARD}"
    if not cfg.is_file():
        return False
    data = cfg.read_text(encoding="utf-8")
    if "CONFIG_ESP_CONSOLE_UART_DEFAULT=y" in data and "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y" not in data:
        return False

    previous = cfg.with_name(cfg.name + ".before-ch343")
    if not previous.exists():
        shutil.copy2(cfg, previous)
        print("[BACKUP] Configuracao antiga salva em", previous)

    data = re.sub(r"(?m)^CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y$",
                  "# CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG is not set", data)
    data = re.sub(r"(?m)^# CONFIG_ESP_CONSOLE_UART_DEFAULT is not set$",
                  "CONFIG_ESP_CONSOLE_UART_DEFAULT=y", data)
    if "CONFIG_ESP_CONSOLE_UART_DEFAULT=y" not in data:
        data += "\nCONFIG_ESP_CONSOLE_UART_DEFAULT=y\n"
    if "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y" in data:
        fail("Falha ao desligar console USB/JTAG no sdkconfig.")
    cfg.write_text(data, encoding="utf-8")
    print("[FIX] Console do teste configurado para UART0 / CH343 (115200 bps).")
    return True


def build(script: Path) -> None:
    if not FW.is_dir():
        fail("Execute prepare antes de compilar.")
    cmd = [sys.executable, str(script)] + idf_args()
    migrated = ensure_uart_console_config()
    if not (FW / f"sdkconfig.{BOARD}").exists():
        execute(cmd + ["set-target", "esp32s3"], cwd=FW)
    if migrated:
        execute(cmd + ["reconfigure"], cwd=FW)
    execute(cmd + ["build"], cwd=FW)
    effective = (FW / f"sdkconfig.{BOARD}").read_text(encoding="utf-8")
    if "CONFIG_ESP_CONSOLE_UART_DEFAULT=y" not in effective or "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y" in effective:
        fail("Build com console incorreto. Configure UART0 em menuconfig antes do flash.")
    print("[OK] Build concluido com console UART0 para CH343. Escolha flash.")


def serial_port(port: str | None) -> str:
    port = (port or input("Porta Serial do JrBot (ex.: COM8): ")).strip().upper()
    if not re.fullmatch(r"COM[0-9]+", port):
        fail(f"Porta invalida: {port!r}. No Windows use, por exemplo, COM8.")
    return port


def flash(script: Path, port: str) -> None:
    print()
    print("[ATENCAO] Gravar vTTS substitui temporariamente o firmware JrBot V2.0 no ESP32.")
    print("[ATENCAO] A branch main fica intacta. Para restaurar o JrBot, grave main por INSTALAR.bat.")
    if input("Digite GRAVAR para confirmar: ").strip() != "GRAVAR":
        fail("Gravacao cancelada pelo usuario.")
    execute([sys.executable, str(script)] + idf_args() + ["-p", port, "flash"], cwd=FW)
    print("[OK] Flash executado. Abra monitor para digitar frases em portugues.")


def monitor(script: Path, port: str) -> None:
    if not FW.is_dir():
        fail("Ambiente do teste ausente. Execute prepare primeiro.")
    print("[INFO] Serial: pressione Enter apos digitar a frase. Para sair, Ctrl+].")
    execute([sys.executable, str(script)] + idf_args() + ["-p", port, "monitor"], cwd=FW)


def main() -> None:
    parser = argparse.ArgumentParser(description="JrBot HW04 / vTTS nu-pt test (isolated)")
    parser.add_argument("action", choices=["prepare", "build", "flash", "monitor", "all", "doctor"])
    parser.add_argument("port", nargs="?", help="Windows COM port, e.g. COM8")
    args = parser.parse_args()
    if args.action == "doctor":
        print("Python:", sys.version.split()[0])
        print("Git:", shutil.which("git") or "nao encontrado")
        try:
            idf_script()
        except RuntimeError as exc:
            print("[PENDENTE]", exc)
        print("Upstream SHA:", UPSTREAM_SHA)
        return
    if args.action in ("prepare", "build", "flash", "all"):
        prepare()
    if args.action == "prepare":
        return
    script = idf_script()
    if args.action in ("build", "flash", "all"):
        build(script)
    if args.action in ("flash", "monitor", "all"):
        port = serial_port(args.port)
        if args.action in ("flash", "all"):
            flash(script, port)
        if args.action in ("monitor", "all"):
            monitor(script, port)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError, ValueError) as exc:
        print("[ERRO]", exc, file=sys.stderr)
        sys.exit(1)
