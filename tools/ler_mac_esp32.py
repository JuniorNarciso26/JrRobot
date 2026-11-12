#!/usr/bin/env python3
"""Read the ESP32-S3 factory MAC over USB without flashing JrBot firmware."""
from __future__ import annotations
import argparse
import re
import subprocess
import sys

MAC_LINE = re.compile(r"(?im)^\s*(?:Base\s+)?MAC(?:\s+Address)?\s*:\s*(?P<mac>(?:[0-9a-f]{2}:){5}[0-9a-f]{2})(?![0-9a-f:])")

class MacReadError(Exception):
    pass

def parse_mac(output: str) -> str:
    match = MAC_LINE.search(output)
    if not match:
        raise MacReadError("Esptool nao retornou um MAC valido.")
    octets = bytes.fromhex(match.group("mac").replace(":", ""))
    if octets == b"\x00" * 6 or octets == b"\xff" * 6 or octets[0] & 1:
        raise MacReadError("MAC invalido para identificacao do dispositivo.")
    return ":".join(f"{b:02X}" for b in octets)

def device_id(mac: str) -> str:
    return "JRBOT-" + mac.replace(":", "")

def read_mac(port: str, runner=subprocess.run) -> str:
    base = [sys.executable, "-m", "esptool"]
    kwargs = {"capture_output": True, "text": True, "errors": "replace"}
    try:
        help_result = runner([*base, "--help"], timeout=15, **kwargs)
        help_text = help_result.stdout + help_result.stderr
        if help_result.returncode:
            raise MacReadError("Instale esptool: python -m pip install esptool")
        if "read-mac" in help_text:
            command = "read-mac"
        elif "read_mac" in help_text:
            command = "read_mac"
        else:
            raise MacReadError("Esptool sem comando read-mac/read_mac.")
        result = runner([*base, "--chip", "esp32s3", "--port", port, command], timeout=45, **kwargs)
    except subprocess.TimeoutExpired as exc:
        raise MacReadError("Tempo esgotado ao consultar a placa USB.") from exc
    if result.returncode:
        raise MacReadError("Falha na porta " + port + ": " + (result.stdout + result.stderr).strip()[-900:])
    return parse_mac(result.stdout + result.stderr)

def list_ports() -> int:
    try:
        from serial.tools import list_ports as serial_ports
    except ImportError:
        print("Instale: python -m pip install esptool", file=sys.stderr)
        return 2
    ports = list(serial_ports.comports())
    if not ports:
        print("Nenhuma porta serial encontrada.")
        return 1
    for item in ports:
        print(f"{item.device} - {item.description}")
    return 0

def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="JrBot: ler MAC eFuse pela USB (somente leitura).")
    parser.add_argument("--port", help="Ex.: COM6 ou /dev/ttyACM0")
    parser.add_argument("--list-ports", action="store_true")
    args = parser.parse_args(argv)
    if args.list_ports:
        return list_ports()
    if not args.port:
        parser.error("informe --port; use --list-ports para listar portas.")
    try:
        mac = read_mac(args.port)
    except MacReadError as exc:
        print("RESULT=FAIL", file=sys.stderr)
        print(f"ERRO: {exc}", file=sys.stderr)
        print("Feche monitores seriais e confira a porta USB e o modo BOOT.", file=sys.stderr)
        return 1
    print(f"MAC_EFUSE={mac}")
    print(f"DEVICE_ID={device_id(mac)}")
    print("RESULT=PASS")
    print("Aviso: a consulta pode reiniciar o ESP32; nao grava firmware nem eFuse.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
