#!/usr/bin/env python3
"""Read existing TTS build metadata. No dependencies, board, build or flash needed.
Only exports selected symbols/configuration/flags, not credentials or certificates.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
SYMBOLS = ("vtts_dot_s8s8", "vtts_dots_s8s8", "vtts_par_run", "vtts_s3_kernel",
           "vtts_set_fast_scratch", "vtts_prof_us", "vtts_prof_n", "jr_tts_say")
SOURCES = ("jr_tts.c", "vtts_synth.c", "vtts_port_s3.c", "vtts_par_freertos.c", "vtts_kernels_ref.c")
KEYS = ("CONFIG_IDF_TARGET", "CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ", "CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240",
        "CONFIG_COMPILER_OPTIMIZATION_PERF", "CONFIG_COMPILER_OPTIMIZATION_SIZE", "CONFIG_COMPILER_OPTIMIZATION_NONE",
        "CONFIG_FREERTOS_HZ", "CONFIG_FREERTOS_UNICORE", "CONFIG_FREERTOS_USE_TIME_SLICING",
        "CONFIG_FREERTOS_IDLE_SHOULD_YIELD", "CONFIG_ESP32S3_DATA_CACHE_SIZE", "CONFIG_ESP32S3_DATA_CACHE_16KB",
        "CONFIG_ESP32S3_DATA_CACHE_32KB", "CONFIG_ESP32S3_DATA_CACHE_64KB", "CONFIG_ESP32S3_DATA_CACHE_LINE_SIZE",
        "CONFIG_ESP32S3_DATA_CACHE_LINE_32B", "CONFIG_ESP32S3_DATA_CACHE_LINE_64B",
        "CONFIG_ESP32S3_INSTRUCTION_CACHE_SIZE", "CONFIG_SPIRAM_MODE_OCT", "CONFIG_SPIRAM_SPEED_80M",
        "CONFIG_SPIRAM_FETCH_INSTRUCTIONS", "CONFIG_SPIRAM_RODATA")


def elf_symbols(path: Path):
    data = path.read_bytes()
    if len(data) < 52 or data[:6] != b"\x7fELF\x01\x01":
        raise ValueError("Esperado ELF32 little-endian do ESP32-S3.")
    header = struct.unpack_from("<16sHHIIIIIHHHHHH", data)
    machine, offset, stride, count = header[2], header[6], header[11], header[12]
    if stride < 40 or not count or offset + stride * count > len(data):
        raise ValueError("Tabela de secoes ELF invalida ou formato estendido nao suportado.")
    sections = [struct.unpack_from("<IIIIIIIIII", data, offset + i * stride) for i in range(count)]
    found = {name: None for name in SYMBOLS}
    for section in sections:
        if section[1] not in (2, 11):
            continue
        if section[6] >= count or section[9] < 16 or section[4] + section[5] > len(data):
            raise ValueError("Tabela de simbolos ELF invalida.")
        strings = sections[section[6]]
        if strings[4] + strings[5] > len(data):
            raise ValueError("Tabela de nomes ELF invalida.")
        names = data[strings[4]:strings[4] + strings[5]]
        for pos in range(section[4], section[4] + section[5] - 15, section[9]):
            name_offset, value, size, info, other, defined = struct.unpack_from("<IIIBBH", data, pos)
            if not defined or name_offset >= len(names):
                continue
            end = names.find(b"\0", name_offset)
            if end < 0:
                continue
            name = names[name_offset:end].decode("ascii", errors="replace")
            if name in found:
                binding = info >> 4
                found[name] = {"binding": {0: "LOCAL", 1: "GLOBAL", 2: "WEAK"}.get(binding, str(binding)),
                               "type": info & 15, "bytes": size}
    return {"machine": machine, "is_xtensa": machine == 94, "sha256": hashlib.sha256(data).hexdigest(),
            "file_bytes": len(data), "symbols": found}


def selected_config(path: Path, header=False):
    if not path.is_file():
        return None
    data = path.read_text(encoding="utf-8", errors="replace")
    result = {}
    for key in KEYS:
        pattern = rf"(?m)^#define\s+{re.escape(key)}\s+(.+)$" if header else rf"(?m)^{re.escape(key)}=(.+)$"
        match = re.search(pattern, data)
        result[key] = match.group(1).strip() if match else (
            "not_set" if not header and f"# {key} is not set" in data else None)
    return result


def build_report(build: Path):
    if not build.is_dir():
        raise ValueError(f"Build nao localizado: {build}. Use --build-dir com a pasta que compilou o firmware instalado.")
    preferred = build / "jrbot_v2.elf"
    candidates = [preferred] if preferred.is_file() else list(build.glob("*.elf"))
    if len(candidates) != 1:
        raise ValueError("Nao foi encontrado um unico ELF nesta pasta. Informe a pasta correta do build.")
    report = {"format": "jrbot-tts-build-v1", "generated_utc": datetime.now(timezone.utc).isoformat(),
              "scope": "existing_local_build_not_runtime_measurement", "elf_name": candidates[0].name,
              "elf": elf_symbols(candidates[0])}
    cache = build / "CMakeCache.txt"
    sdkconfig = build.parent / "sdkconfig.develop_vtts"
    if cache.is_file():
        match = re.search(r"(?m)^SDKCONFIG:[^=]+=(.+)$", cache.read_text(encoding="utf-8", errors="replace"))
        if match:
            candidate = Path(match.group(1).strip())
            sdkconfig = candidate if candidate.is_absolute() else build.parent / candidate
    report["sdkconfig"] = selected_config(sdkconfig)
    report["generated_config"] = selected_config(build / "config/sdkconfig.h", header=True)
    commands = build / "compile_commands.json"
    report["compile_units"] = []
    if commands.is_file():
        for unit in json.loads(commands.read_text(encoding="utf-8")):
            filename = str(unit.get("file", "")).replace("\\", "/").rsplit("/", 1)[-1]
            if filename not in SOURCES:
                continue
            command = unit.get("command") or " ".join(unit.get("arguments", []))
            report["compile_units"].append({"source": filename,
                "optimization_flags": re.findall(r"-O(?:[0-3sgz]|fast)\b", command),
                "vtts_definitions": re.findall(r"-D(VTTS_[A-Z0-9_]+(?:=[^\s\"']+)?)", command)})
    revision = ROOT / "firmware/core/jr_firmware_revision_local.h"
    report["local_generated_git_stamp"] = None
    if revision.is_file():
        match = re.search(r'JR_FIRMWARE_GIT_COMMIT\s+"([a-fA-F0-9]+)"', revision.read_text())
        if match:
            report["local_generated_git_stamp"] = match.group(1)
    marker = ROOT / "firmware/vendor/vtts_src/.vtts_revision"
    report["local_vendor_revision"] = marker.read_text().strip() if marker.is_file() else None
    report["interpretation"] = [
        "WEAK em dot/dots/par_run indica fallback generico selecionado; GLOBAL exige conferencia com fontes/flags.",
        "GLOBAL nao comprova que todas as chamadas usam SIMD, nem mede ganho real.",
        "Simbolo ausente pode ter sido removido pelo linker; sozinho nao prova falha.",
        "Config/cabecalho local e ELF podem estar desatualizados: comparar com git do boot instalado.",
        "Este relatorio nao compila, nao grava e nao mede velocidade nem prioridade efetiva em runtime.",
    ]
    return report


def main():
    parser = argparse.ArgumentParser(description="Diagnostico somente leitura do build TTS existente")
    parser.add_argument("--build-dir", type=Path, default=ROOT / "firmware/b-develop-vtts")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    try:
        report = build_report(args.build_dir.resolve())
        output = args.output or ROOT / ".tts-benchmark-results" / f"tts-build-{datetime.now(timezone.utc):%Y%m%dT%H%M%SZ}.json"
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        print("[OK] Relatorio:", output)
        for name in SYMBOLS[:4]:
            print(name + ":", report["elf"]["symbols"][name])
        print("Envie o JSON. Nenhuma compilacao ou gravacao foi executada.")
    except (OSError, ValueError, struct.error) as exc:
        parser.exit(1, f"[ERRO] {exc}\n")


if __name__ == "__main__":
    main()
