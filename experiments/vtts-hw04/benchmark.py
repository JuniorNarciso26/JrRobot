#!/usr/bin/env python3
"""Fixed spoken baseline. Only sends console commands; never patches firmware.

Film Enter through the end of speech. Host/serial timestamps are supplementary,
not a measurement of when sound actually leaves the speaker.
"""
from __future__ import annotations

from datetime import datetime, timezone
import hashlib
from pathlib import Path
import time

BASE = Path(__file__).resolve().parent
TEST_ID = "JR_TTS_LONG_01"
SETTINGS = ("model s", "lang pt", "vol 75", "gain 6", "speed 1", "expr 1",
            "debuzz 0", "cores 2", "chunk 128", "first 12")


def standard_phrase() -> str:
    text = (BASE / "frase_padrao.txt").read_text(encoding="utf-8").strip()
    if not text or "\n" in text or "\r" in text or len(text.encode("utf-8")) > 1200:
        raise RuntimeError("Frase padrao vazia, multilinha ou maior que 1.200 bytes.")
    return text


def read_done(device, record, timeout: float, start: float | None = None) -> list[str]:
    deadline = time.monotonic() + timeout
    lines = []
    pending = bytearray()
    while time.monotonic() < deadline:
        raw = device.read(1)
        if not raw:
            continue
        if raw != b"\n":
            if len(pending) >= 16384:
                raise RuntimeError("Linha Serial excedeu 16 KiB; teste interrompido.")
            pending.extend(raw)
            continue
        line = pending.decode("utf-8", errors="replace").strip()
        pending.clear()
        if not line:
            continue
        prefix = "SERIAL" if start is None else f"SERIAL +{(time.perf_counter() - start) * 1000:.0f}ms"
        record(f"{prefix} {line}")
        if line.startswith("ERROR"):
            raise RuntimeError(f"Firmware recusou o comando: {line}")
        if "task_wdt" in line or "Guru Meditation" in line or "rst:" in line:
            raise RuntimeError("Watchdog ou reinicio durante teste. Log preservado; nao repetir automaticamente.")
        if line == "DONE":
            return lines
        lines.append(line)
    raise RuntimeError("Tempo esgotado esperando DONE. Log preservado; frase nao sera reenviada.")


def command(device, text: str, record) -> list[str]:
    payload = (text + "\n").encode("utf-8")
    record(f"COMMAND {text}")
    if device.write(payload) != len(payload):
        raise RuntimeError("Escrita Serial incompleta.")
    device.flush()
    return read_done(device, record, 15)


def run(port: str) -> None:
    try:
        import serial
    except ImportError as exc:
        raise RuntimeError("pyserial ausente. Use o Python do terminal ESP-IDF ou execute: python -m pip install pyserial") from exc
    text = standard_phrase()
    digest = hashlib.sha256(text.encode("utf-8")).hexdigest()
    results = BASE / ".work" / "results"
    results.mkdir(parents=True, exist_ok=True)
    path = results / f"{TEST_ID}_{datetime.now(timezone.utc):%Y%m%dT%H%M%S%fZ}.txt"
    with path.open("w", encoding="utf-8", buffering=1) as log:
        def record(line: str) -> None:
            print(line, flush=True)
            log.write(line + "\n")
        record(f"TEST_ID={TEST_ID} stage=isolated upstream=010cd22fd51f58dfa2414824cb8d6345e70cafc6")
        record(f"PHRASE_SHA256={digest} utf8_bytes={len(text.encode('utf-8'))}")
        record("PHRASE=" + text)
        record("TIMING=video_enter_to_acoustic_onset_and_end; serial_is_supplementary")
        print(f"Log: {path}\nFeche qualquer monitor Serial que esteja usando {port}.", flush=True)
        device = serial.Serial(port=None, baudrate=115200, timeout=0.2, write_timeout=3)
        try:
            # Avoid an intentional DTR/RTS reset when opening an already running lab.
            device.dtr = False
            device.rts = False
            device.port = port
            device.open()
            device.reset_input_buffer()
            # A harmless query must prove this is the isolated upstream console.
            identity = command(device, "model", record)
            if not any(line.startswith("model ") and "token set pt-en" in line for line in identity):
                raise RuntimeError("Console vTTS pt-en isolado nao identificado. Grave a branch de teste antes de medir.")
            for setting in SETTINGS:
                response = command(device, setting, record)
                name, value = setting.split()
                if not any(line.lower().startswith(name + " ") and
                           line.lower().split()[1] == ("1.00" if name in ("speed", "expr") else
                                                      "6.0" if name == "gain" else value)
                           for line in response):
                    raise RuntimeError(f"Nao foi possivel confirmar o ajuste {setting!r}.")
            command(device, "prof", record)  # Clear previous counters before the timed speech.
            print("\nPRONTO. Filme o teclado, a tela e o robo, com o som audivel.\n"
                  "Pressione ENTER uma unica vez para enviar a frase padrao.", flush=True)
            input()
            payload = (text + "\n").encode("utf-8")
            enter_utc = datetime.now(timezone.utc).isoformat()
            start = time.perf_counter()
            print(f"JR_TTS_BENCH_BEGIN {TEST_ID} -- FRASE ENVIADA", flush=True)
            if device.write(payload) != len(payload):
                raise RuntimeError("Envio incompleto; frase nao sera repetida automaticamente.")
            device.flush()
            record(f"HOST_SEND_COMPLETE elapsed_ms={(time.perf_counter() - start) * 1000:.1f}")
            record(f"HOST_ENTER_UTC={enter_utc} start=before_serial_write")
            read_done(device, record, 600, start)
            record(f"JR_TTS_BENCH_DONE host_elapsed_ms={(time.perf_counter() - start) * 1000:.1f}")
            command(device, "prof", record)
            record("RESULT=command_completed; acoustic_onset_continuity_and_end_require_video")
        except (Exception, KeyboardInterrupt) as exc:
            record(f"TEST_ABORTED {type(exc).__name__}: {exc}")
            raise
        finally:
            device.close()
    print(f"\nTeste encerrado. Envie o video e este log: {path}")
