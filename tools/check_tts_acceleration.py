#!/usr/bin/env python3
"""Reject an ESP32-S3 vTTS ELF that selected portable weak fallback functions."""
from __future__ import annotations
import argparse
from pathlib import Path
from tts_build_diagnostics import elf_symbols

FUNCTIONS = ("vtts_dot_s8s8", "vtts_dots_s8s8", "vtts_par_run")


def require_acceleration(elf):
    if not elf.get("is_xtensa"):
        raise ValueError("O ELF nao e Xtensa/ESP32-S3.")
    symbols = elf["symbols"]
    for name in FUNCTIONS:
        symbol = symbols.get(name)
        if not symbol or symbol.get("binding") != "GLOBAL" or symbol.get("type") != 2 or not symbol.get("bytes"):
            raise ValueError(f"{name}: esperado GLOBAL/FUNC otimizado, encontrado {symbol}. Build TTS rejeitado.")
    kernel = symbols.get("vtts_s3_kernel")
    if not kernel or kernel.get("binding") != "GLOBAL" or kernel.get("type") != 1 or not kernel.get("bytes"):
        raise ValueError(f"vtts_s3_kernel ausente/invalido: {kernel}. Port S3 nao confirmado.")


def main():
    parser = argparse.ArgumentParser(description="Validar funcoes S3/two-core antes de aceitar o build vTTS")
    parser.add_argument("--elf", type=Path, required=True)
    args = parser.parse_args()
    try:
        elf = elf_symbols(args.elf)
        require_acceleration(elf)
    except (OSError, ValueError) as exc:
        parser.exit(1, f"[ERRO] JR_TTS_ACCELERATION_CHECK: {exc}\n")
    print("[OK] JR_TTS_ACCELERATION_CHECK: SIMD S3 + parallel GLOBAL; fallback WEAK rejeitado.")
    print("[INFO] Verificacao de link concluida; desempenho e continuidade ainda precisam de teste fisico.")


if __name__ == "__main__":
    main()
