#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import sys
import time

try:
    import serial
    from serial.tools import list_ports
except ImportError as exc:
    raise SystemExit("ERRO: pyserial nao encontrado. Rode: python -m pip install pyserial") from exc

BAUD = 115200
PINS = [
    (1, "OLED SDA"),
    (2, "OLED SCL"),
    (41, "MIC SD"),
    (42, "MAX98357A DIN"),
    (47, "I2S WS/LRC"),
    (21, "I2S BCLK/SCK"),
]


def port_key(name: str) -> tuple[int, str]:
    match = re.fullmatch(r"COM(\d+)", name.upper())
    return (int(match.group(1)) if match else 999999, name.upper())


def read_lines(ser: serial.Serial, seconds: float) -> list[str]:
    deadline = time.monotonic() + seconds
    lines: list[str] = []
    buf = bytearray()
    while time.monotonic() < deadline:
        data = ser.read(256)
        if not data:
            time.sleep(0.02)
            continue
        buf.extend(data)
        while b"\n" in buf:
            raw, _, rest = buf.partition(b"\n")
            buf = bytearray(rest)
            text = raw.decode("utf-8", errors="replace").strip()
            if text:
                lines.append(text)
    return lines


def probe(port: str) -> serial.Serial | None:
    try:
        ser = serial.Serial(port, BAUD, timeout=0.08, write_timeout=1.0)
    except Exception:
        return None
    try:
        time.sleep(0.25)
        ser.reset_input_buffer()
        ser.write(b"status\n")
        ser.flush()
        for line in read_lines(ser, 1.8):
            if line.startswith("WIRE_STATUS") or line.startswith("WIRE_TEST_READY"):
                return ser
    except Exception:
        pass
    try:
        ser.close()
    except Exception:
        pass
    return None


def connect(preferred: str | None) -> tuple[serial.Serial, str]:
    candidates: list[str] = []
    if preferred:
        candidates.append(preferred.upper())
    detected = sorted({p.device.upper() for p in list_ports.comports()}, key=port_key)
    for port in detected:
        if port not in candidates:
            candidates.append(port)

    print("\nProcurando o firmware TESTA_FIO...")
    for port in candidates:
        print(f"  testando {port}...", end="", flush=True)
        ser = probe(port)
        if ser is not None:
            print(" OK")
            return ser, port
        print(" sem resposta")
    raise SystemExit("ERRO: firmware TESTA_FIO nao respondeu em nenhuma porta COM. Desconecte outros monitores seriais e tente novamente.")


def command(ser: serial.Serial, text: str, timeout: float = 2.0) -> str:
    ser.reset_input_buffer()
    ser.write((text.strip() + "\n").encode("ascii"))
    ser.flush()
    deadline = time.monotonic() + timeout
    buf = bytearray()
    while time.monotonic() < deadline:
        data = ser.read(256)
        if data:
            buf.extend(data)
            while b"\n" in buf:
                raw, _, rest = buf.partition(b"\n")
                buf = bytearray(rest)
                line = raw.decode("utf-8", errors="replace").strip()
                if line.startswith("WIRE_"):
                    return line
        else:
            time.sleep(0.02)
    raise RuntimeError("sem resposta do firmware")


def show_menu(port: str) -> None:
    print("\n========================================")
    print("TESTA_FIO - JrBot ESP32-S3")
    print(f"Porta de comandos: {port}")
    print("========================================")
    for index, (pin, label) in enumerate(PINS, start=1):
        print(f"  [{index}] GPIO{pin:<2} - {label}")
    print("  [0] Sair")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default="")
    args = parser.parse_args()

    ser, port = connect(args.port or None)
    try:
        try:
            print(command(ser, "off"))
        except Exception:
            pass

        while True:
            show_menu(port)
            choice = input("\nEscolha o pino: ").strip().lower()
            if choice in {"0", "q", "sair", "exit"}:
                try:
                    print(command(ser, "off"))
                except Exception:
                    pass
                print("Teste encerrado. Todos os pinos de teste ficaram em alta impedancia.")
                return 0
            if not choice.isdigit() or not (1 <= int(choice) <= len(PINS)):
                print("Opcao invalida.")
                continue

            pin, label = PINS[int(choice) - 1]
            try:
                reply = command(ser, f"pin {pin}")
            except Exception as exc:
                print(f"ERRO: {exc}")
                return 2
            if not reply.startswith("WIRE_OK"):
                print("ERRO: " + reply)
                continue

            print("\n----------------------------------------")
            print(f"GPIO{pin} - {label}")
            print("Saida de teste ativa: aproximadamente 3.3 V para medicao com multimetro.")
            print("Meça entre o fio/pino e GND.")
            print("Este sinal usa pull-up seguro e NAO serve para alimentar dispositivos.")
            input("Quando terminar a medicao, pressione ENTER...")
            try:
                command(ser, "off")
            except Exception as exc:
                print(f"AVISO: nao consegui desligar pelo comando: {exc}")
                return 2
    finally:
        try:
            ser.close()
        except Exception:
            pass


if __name__ == "__main__":
    raise SystemExit(main())
