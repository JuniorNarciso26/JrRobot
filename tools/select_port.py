#!/usr/bin/env python3
"""Seleciona uma porta COM detectada e grava a escolha para o instalador."""
from __future__ import annotations

import argparse
import re
from pathlib import Path

try:
    from serial.tools import list_ports
except ImportError as exc:
    raise SystemExit("ERRO: pyserial nao encontrado no Python atual.") from exc


def port_key(name: str) -> tuple[int, str]:
    match = re.fullmatch(r"COM(\d+)", name.upper())
    return (int(match.group(1)) if match else 999999, name.upper())


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    ports = sorted({p.device.upper() for p in list_ports.comports()}, key=port_key)
    if not ports:
        print("ERRO: nenhuma porta serial foi detectada.")
        return 2

    print("\n========================================")
    print("JrBot V2 - escolha a porta da placa")
    print("========================================")
    for index, port in enumerate(ports, start=1):
        print(f"  [{index}] {port}")

    while True:
        answer = input("\nDigite o numero da porta: ").strip()
        if answer.isdigit():
            selected = int(answer)
            if 1 <= selected <= len(ports):
                port = ports[selected - 1]
                Path(args.output).write_text(port + "\n", encoding="ascii")
                print(f"Porta selecionada: {port}")
                return 0
        print("Opcao invalida. Escolha um numero da lista.")


if __name__ == "__main__":
    raise SystemExit(main())
