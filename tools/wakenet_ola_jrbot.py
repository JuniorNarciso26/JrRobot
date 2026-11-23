#!/usr/bin/env python3
"""Check the data needed to request a custom 'Olá JrBot' WakeNet model.
This tool does NOT train a neural network or modify the ESP-SR upstream model.
Standard-library only (Windows / Linux).
"""
from __future__ import annotations

import argparse
import json
import wave
from pathlib import Path

EXPECTED_SR = 16000
PHRASE = "Olá JrBot"


def check_corpus(root: Path) -> int:
    if not root.exists():
        print(f"[INFO] Pasta ausente: {root}")
        print("[INFO] Adicione WAVs para iniciar a coleta; nenhum modelo foi treinado.")
        return 2
    wavs = sorted(root.rglob("*.wav"))
    if not wavs:
        print(f"[INFO] Nenhum arquivo .wav encontrado em {root}")
        return 2
    good, bad = 0, 0
    speakers = set()
    for file in wavs:
        try:
            with wave.open(str(file), "rb") as audio:
                channels = audio.getnchannels()
                width = audio.getsampwidth()
                rate = audio.getframerate()
                frames = audio.getnframes()
            valid = (channels == 1 and width == 2 and rate == EXPECTED_SR and frames > 0)
            if valid:
                good += 1
                speakers.add(file.parent.name)
            else:
                bad += 1
                print(f"[ERRO] {file}: channels={channels}, sample_width={width}, rate={rate}, frames={frames}")
        except (wave.Error, EOFError, OSError) as exc:
            bad += 1
            print(f"[ERRO] {file}: {exc}")
    print(f"[RESUMO] frase={PHRASE} WAV_validos={good} WAV_invalidos={bad} pastas_de_locutor={len(speakers)}")
    print("[NOTA] Nao verifica consentimento, qualidade acustica, variedade ou rotulos de voz.")
    print("[NOTA] Validade de WAV nao significa prontidao para treinar WakeNet.")
    return 0 if not bad else 1


def check_model(root: Path) -> int:
    if not root.exists():
        print(f"[PENDENTE] Diretorio de modelo nao existe: {root}")
        return 2
    dirs = sorted(p for p in root.iterdir() if p.is_dir() and (p / "_MODEL_INFO_").is_file())
    if not dirs:
        print("[PENDENTE] Nenhum diretorio WakeNet com _MODEL_INFO_ encontrado.")
        return 2
    for folder in dirs:
        print(f"[MODELO] {folder.name}: _MODEL_INFO_ presente; integridade/compatibilidade nao verificada")
    print("[INFO] A integracao exige validar o modelo no ESP32-S3 e selecionar CONFIG_ESP_SR_EXTERNAL_MODEL_PATH.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Preparacao WakeNet personalizado 'Olá JrBot'")
    parser.add_argument("action", choices=["corpus", "modelo", "status"])
    parser.add_argument("--corpus", type=Path, default=Path("experiments/ola-jrbot-wakenet/dataset"))
    parser.add_argument("--model-dir", type=Path, default=Path("experiments/ola-jrbot-wakenet/models"))
    args = parser.parse_args()
    if args.action == "corpus":
        return check_corpus(args.corpus)
    if args.action == "modelo":
        return check_model(args.model_dir)
    print(json.dumps({
        "wake_word": PHRASE,
        "target": "ESP32-S3 N16R8 HW04",
        "engine": "WakeNet10 (model customizado ainda nao disponivel)",
        "baseline": "wn10_pt_olaesp funciona fisicamente",
        "corpus": str(args.corpus),
        "model_dir": str(args.model_dir),
        "trainable_here": False,
        "required_external_step": "solicitar modelo treinado a Espressif ou desenvolver pipeline de treinamento independente",
    }, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
