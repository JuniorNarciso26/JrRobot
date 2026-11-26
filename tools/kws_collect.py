#!/usr/bin/env python3
"""Serial monitor and reviewed WAV collection for the isolated HW04 experiment."""
from __future__ import annotations

import argparse
import array
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import queue
import re
import sys
import tempfile
import threading
import time
import unicodedata
import uuid
import wave

RATE = 16000
PCM_BYTES = RATE * 2


def fnv1a(data: bytes) -> int:
    value = 2166136261
    for byte in data:
        value = ((value ^ byte) * 16777619) & 0xffffffff
    return value


class Recording:
    """Fail closed on missing/repeated/corrupt/wrong-sized frames, never save a partial clip."""
    def __init__(self, ident: str):
        self.ident = ident
        self.started = False
        self.done = False
        self.sequence = 0
        self.pcm = bytearray()

    def feed(self, line: str) -> bool:
        if not line.startswith("JR_KWS_"):
            return False
        fields = dict(re.findall(r"([a-z]+)=([^ ]+)", line))
        if fields.get("id") != self.ident:
            return False
        verb = line.split(" ", 1)[0]
        if verb == "JR_KWS_ERROR":
            raise RuntimeError("Firmware recusou gravacao: " + fields.get("reason", "erro"))
        if self.done:
            raise RuntimeError("Dados extras apos fim da gravacao")
        if verb == "JR_KWS_BEGIN":
            if self.started or (fields.get("rate"), fields.get("samples"), fields.get("bytes")) != ("16000", "16000", "32000"):
                raise RuntimeError("Cabecalho de audio invalido")
            self.started = True
        elif verb == "JR_KWS_CAPTURED":
            if not self.started:
                raise RuntimeError("Captura sem cabecalho")
        elif verb == "JR_KWS_DATA":
            if not self.started or fields.get("seq") != str(self.sequence):
                raise RuntimeError("Bloco Serial ausente ou fora de ordem; gravacao descartada")
            text = fields.get("hex", "")
            if not re.fullmatch(r"[0-9a-f]{512}", text):
                raise RuntimeError("Bloco de audio corrompido")
            self.pcm.extend(bytes.fromhex(text))
            self.sequence += 1
            if len(self.pcm) > PCM_BYTES:
                raise RuntimeError("Audio excede um segundo")
        elif verb == "JR_KWS_END":
            if not self.started or len(self.pcm) != PCM_BYTES or fields.get("fnv") != f"{fnv1a(self.pcm):08x}":
                raise RuntimeError("Audio incompleto ou checksum incorreto; nada salvo")
            self.done = True
        return self.done


def safe_name(text: str) -> str:
    name = unicodedata.normalize("NFKD", text.strip().lower()).encode("ascii", "ignore").decode()
    if not re.fullmatch(r"[a-z][a-z0-9_-]{0,31}", name):
        raise ValueError("Use de 1 a 32 letras/numeros, hifen ou sublinhado; comece com uma letra.")
    return name


def audio_stats(pcm: bytes) -> dict:
    samples = array.array("h", pcm)
    if sys.byteorder != "little":
        samples.byteswap()
    return {"peak": max(abs(v) for v in samples),
            "rms": round(math.sqrt(sum(v*v for v in samples) / len(samples)), 1),
            "clipped_samples": sum(v in (-32768, 32767) for v in samples)}


def write_wav(path: Path, pcm: bytes) -> None:
    with path.open("xb") as stream:
        with wave.open(stream, "wb") as wav:
            wav.setnchannels(1)
            wav.setsampwidth(2)
            wav.setframerate(RATE)
            wav.writeframes(pcm)


def save_recording(root: Path, label: str, speaker: str, recording: Recording) -> Path:
    if not recording.done or len(recording.pcm) != PCM_BYTES:
        raise ValueError("Somente uma gravacao completa pode ser salva")
    label, speaker = safe_name(label), safe_name(speaker)
    folder = root / label
    folder.mkdir(parents=True, exist_ok=True)
    path = folder / f"{speaker}-{uuid.uuid4().hex}.wav"
    pcm = bytes(recording.pcm)
    metadata = {"label": label, "speaker": speaker, "rate": RATE, "samples": RATE,
                "bits": 16, "channels": 1, "source": "jrbot_hw04_serial_pcm",
                "captured_utc": datetime.now(timezone.utc).isoformat(),
                "sha256": hashlib.sha256(pcm).hexdigest(), **audio_stats(pcm)}
    write_wav(path, pcm)
    try:
        with path.with_suffix(".json").open("x", encoding="utf-8") as out:
            json.dump(metadata, out, indent=2, ensure_ascii=False)
    except OSError:
        path.unlink()
        raise
    return path


class SerialMonitor:
    def __init__(self, port):
        self.port = port
        self.lines = queue.Queue(maxsize=2048)
        self.stop = threading.Event()
        self.error = None
        self.thread = threading.Thread(target=self._read, daemon=True)
        self.thread.start()

    def _read(self):
        pending = bytearray()
        try:
            while not self.stop.is_set():
                pending.extend(self.port.read(1024))
                while b"\n" in pending:
                    line, _, pending = pending.partition(b"\n")
                    text = line.decode("ascii", errors="replace").rstrip("\r")
                    if self.lines.full():
                        try:
                            self.lines.get_nowait()
                        except queue.Empty:
                            pass
                    self.lines.put_nowait(text)
                if len(pending) > 4096:
                    raise RuntimeError("Linha Serial invalida; reconecte o coletor")
        except Exception as exc:
            self.error = exc

    def command(self, text):
        while True:
            try:
                self.lines.get_nowait()
            except queue.Empty:
                break
        payload = (text + "\n").encode("ascii")
        if self.port.write(payload) != len(payload):
            raise RuntimeError("Escrita Serial incompleta; nada salvo")
        self.port.flush()

    def next_line(self, deadline):
        while time.monotonic() < deadline:
            if self.error:
                raise RuntimeError("Falha lendo Serial; reconecte o robô") from self.error
            try:
                return self.lines.get(timeout=0.1)
            except queue.Empty:
                continue
        raise TimeoutError("Tempo de espera excedido. Confira firmware, porta e microfone; nada salvo.")

    def handshake(self):
        for _ in range(3):
            self.command("kws_info")
            deadline = time.monotonic() + 4
            try:
                while True:
                    if self.next_line(deadline) == "JR_KWS_INFO v=1 rate=16000 samples=16000":
                        return
            except TimeoutError:
                continue
        raise RuntimeError("Firmware sem suporte a coleta. Reinstale test/tflite-yes-no pelo INSTALAR.bat.")

    def capture(self) -> Recording:
        recording = Recording(uuid.uuid4().hex[:16])
        self.command("kws_record " + recording.ident)
        deadline = time.monotonic() + 20
        announced = False
        while True:
            line = self.next_line(deadline)
            if recording.feed(line):
                return recording
            if recording.started and not announced:
                print("GRAVANDO: fale agora (1 segundo)...", flush=True)
                announced = True
            if line.startswith("JR_KWS_CAPTURED id=" + recording.ident):
                print("Capturado. Recebendo audio pela USB...", flush=True)

    def close(self):
        self.stop.set()
        self.thread.join(timeout=1)
        self.port.close()


def interactive(monitor: SerialMonitor, output: Path) -> None:
    print("\nCOLETA DE VOZ - microfone do JrBot, 1 segundo, WAV mono 16 kHz.")
    print("O bip fica bloqueado durante a captura e a transferencia.")
    print("Destino:", output)
    speaker = safe_name(input("Identificador da pessoa [pessoa01]: ") or "pessoa01")
    while True:
        label = input("\nPalavra: sim / nao / silence / unknown / outra | m=monitor | q=sair: ").strip()
        if label.lower() == "q":
            return
        if label.lower() == "m":
            print("Monitor por 10 segundos (Ctrl+C encerra o coletor).")
            deadline = time.monotonic() + 10
            try:
                while True:
                    print(monitor.next_line(deadline))
            except TimeoutError:
                pass
            continue
        try:
            label = safe_name(label)
        except ValueError as exc:
            print(exc)
            continue
        while True:
            print("\nClasse:", label)
            print("Fique quieto." if label == "silence" else
                  "Fale outra palavra/som, diferente dos comandos." if label == "unknown" else
                  f"Diga '{label}' uma vez quando aparecer GRAVANDO.")
            if input("ENTER para preparar; v para voltar: ").strip().lower() == "v":
                break
            for value in (3, 2, 1):
                print(value, flush=True)
                time.sleep(1)
            try:
                recording = monitor.capture()
            except (RuntimeError, TimeoutError) as exc:
                print("[ERRO]", exc)
                continue
            stats = audio_stats(recording.pcm)
            print(f"Recebido e verificado: pico={stats['peak']}/32768 RMS={stats['rms']} saturadas={stats['clipped_samples']}")
            with tempfile.TemporaryDirectory(prefix="jrbot-voz-") as tmp:
                preview = Path(tmp) / "preview.wav"
                write_wav(preview, recording.pcm)
                while True:
                    choice = input("o=ouvir | s=salvar | ENTER=descartar: ").strip().lower()
                    if choice == "o":
                        if sys.platform == "win32":
                            import winsound
                            winsound.PlaySound(str(preview), winsound.SND_FILENAME)
                        else:
                            print("Preview WAV:", preview)
                        continue
                    if choice == "s":
                        path = save_recording(output, label, speaker, recording)
                        print("[SALVO]", path)
                        print("Total desta classe:", len(list(path.parent.glob("*.wav"))))
                        break
                    if choice == "":
                        print("Descartado.")
                        break


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", help="Porta COM; se omitida, mostra a lista")
    default = Path(os.environ.get("LOCALAPPDATA", str(Path.home()))) / "JrBot" / "voice-dataset"
    parser.add_argument("--output", type=Path, default=default)
    args = parser.parse_args()
    try:
        import serial
        from serial.tools import list_ports
        port_name = args.port
        if not port_name:
            ports = list(list_ports.comports())
            if not ports:
                raise RuntimeError("Nenhuma porta Serial detectada")
            for index, port in enumerate(ports, 1):
                print(index, port.device, port.description)
            selected = int(input("Numero da porta: "))
            if not 1 <= selected <= len(ports):
                raise ValueError("Porta fora da lista")
            port_name = ports[selected - 1].device
        port = serial.Serial(port=None, baudrate=115200, timeout=0.1, write_timeout=2)
        port.dtr = False
        port.rts = False
        port.port = port_name
        port.open()
        monitor = SerialMonitor(port)
        try:
            print("Verificando firmware de coleta...")
            monitor.handshake()
            interactive(monitor, args.output.resolve())
        finally:
            monitor.close()
        return 0
    except (OSError, RuntimeError, TimeoutError, ValueError, ImportError) as exc:
        print("[ERRO]", exc, "\nFeche qualquer outro monitor/painel que esteja usando a mesma COM.")
        return 1
    except (KeyboardInterrupt, EOFError):
        print("\nColetor encerrado; amostras ja salvas foram preservadas.")
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
