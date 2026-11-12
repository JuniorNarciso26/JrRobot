"""Build-time RAM inventory from the linked ELF and GNU map (stdlib only).

Generated files live exclusively in the ignored build directory. The installer
relinks the flash-resident JSON and verifies RAM sections before flashing.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import struct
import subprocess
import sys
from collections import defaultdict
from pathlib import Path


def memory_kind(address: int) -> str | None:
    if 0x3FC88000 <= address < 0x3FD00000 or 0x40370000 <= address < 0x403E0000:
        return "sram"
    if 0x600FE000 <= address < 0x60100000 or 0x50000000 <= address < 0x50002000:
        return "rtc"
    if 0x3C000000 <= address < 0x3E000000:
        return "psram_or_mapping"
    return None


def parse_elf(blob: bytes) -> tuple[list[dict], list[dict], dict[str, int]]:
    if len(blob) < 52 or blob[:6] != b"\x7fELF\x01\x01":
        raise ValueError("ELF deve ser ELF32 little-endian (ESP32-S3)")
    header = struct.unpack_from("<16sHHIIIIIHHHHHH", blob)
    shoff, shentsize, shnum, shstrndx = header[6], header[11], header[12], header[13]
    if shentsize != 40 or not shnum or shstrndx >= shnum or shoff + shnum * shentsize > len(blob):
        raise ValueError("Tabela de secoes ELF invalida")
    raw = [struct.unpack_from("<IIIIIIIIII", blob, shoff + i * shentsize) for i in range(shnum)]

    def contents(section):
        offset, size = section[4], section[5]
        if offset + size > len(blob):
            raise ValueError("Secao ELF fora do arquivo")
        return blob[offset:offset + size]

    def text(table: bytes, offset: int) -> str:
        if offset >= len(table):
            return ""
        return table[offset:table.find(b"\0", offset) if b"\0" in table[offset:] else len(table)].decode("utf-8", "replace")

    names = contents(raw[shstrndx])
    sections = []
    by_index = {}
    for index, sec in enumerate(raw):
        name = text(names, sec[0])
        kind = memory_kind(sec[3])
        # The dummy DRAM section aliases IRAM; counting it doubles the same SRAM.
        if not (sec[2] & 2) or not sec[5] or not kind or "dummy" in name:
            continue
        if kind == "psram_or_mapping" and not name.startswith((".ext_ram", ".external")):
            continue
        item = {"name": name, "address": sec[3], "size": sec[5], "kind": kind,
                "category": "bss" if sec[1] == 8 else "code" if sec[2] & 4 else "data"}
        sections.append(item)
        by_index[index] = item
    symbols, values = [], {}
    seen = set()
    for sec in raw:
        if sec[1] != 2:
            continue
        if sec[6] >= shnum or sec[9] != 16 or sec[5] % 16:
            raise ValueError("Tabela de simbolos ELF invalida")
        strings = contents(raw[sec[6]])
        for offset in range(sec[4], sec[4] + sec[5], 16):
            name_at, address, size, info, _, section_index = struct.unpack_from("<IIIBBH", blob, offset)
            name = text(strings, name_at)
            if name:
                values[name] = address
            section = by_index.get(section_index)
            if not section or not size or info & 15 not in (1, 2) or not name:
                continue
            if not section["address"] <= address < address + size <= section["address"] + section["size"]:
                continue
            key = (name, address, size, section["name"])
            if key in seen:
                continue
            seen.add(key)
            symbols.append({"name": name, "address": address, "size": size,
                            "section": section["name"], "kind": section["kind"], "owner": "Nao identificado no map"})
    return sections, symbols, values


def parse_map(source: str, sections: list[dict]) -> list[dict]:
    if "Linker script and memory map" not in source:
        return []
    source = source.split("Linker script and memory map", 1)[1]
    entries, seen = [], set()
    pending = None
    one_line = re.compile(r"^\s+(\.[^\s]+)\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)\s+(.+)$")
    continuation = re.compile(r"^\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)\s+(.+)$")
    for line in source.splitlines():
        match = one_line.match(line)
        if match:
            input_name, address, size, owner = match.groups()
        elif pending and (match := continuation.match(line)):
            input_name, (address, size, owner) = pending, match.groups()
        else:
            pending = line.strip() if re.fullmatch(r"\s+\.[^\s]+\s*", line) else None
            continue
        pending = None
        if not re.search(r"(?:\.a\([^)]*\)|\.(?:o|obj))$", owner.strip()):
            continue
        # Keep archive/member identity without embedding local user directories.
        owner = owner.strip().replace("\\", "/")
        archive_owner = re.search(r"([^/]+\.a\(.+\))$", owner)
        owner = archive_owner.group(1) if archive_owner else owner.rsplit("/", 1)[-1]
        address, size = int(address, 16), int(size, 16)
        if not size:
            continue
        for section in sections:
            start, end = max(address, section["address"]), min(address + size, section["address"] + section["size"])
            if end <= start:
                continue
            key = (start, end, owner, section["name"])
            if key not in seen:
                seen.add(key)
                entries.append({"address": start, "size": end - start, "owner": owner.strip(),
                                "section": section["name"], "kind": section["kind"], "input": input_name})
    return entries


def inventory(elf: bytes, map_text: str, version: str) -> dict:
    sections, symbols, values = parse_elf(elf)
    entries = parse_map(map_text, sections)
    for symbol in symbols:
        owner = next((e["owner"] for e in entries if e["section"] == symbol["section"] and
                      e["address"] <= symbol["address"] < e["address"] + e["size"]), None)
        if owner:
            symbol["owner"] = owner
    components = defaultdict(int)
    component_categories = defaultdict(lambda: defaultdict(int))
    objects = defaultdict(lambda: defaultdict(int))
    covered, overlap = 0, 0
    for section in sections:
        if section["kind"] != "sram":
            continue
        end = section["address"]
        for entry in sorted((e for e in entries if e["section"] == section["name"]), key=lambda e: e["address"]):
            start = max(end, entry["address"])
            stop = entry["address"] + entry["size"]
            overlap += max(0, min(end, stop) - entry["address"])
            if stop > start:
                owner = entry["owner"].replace("\\", "/")
                archive = re.search(r"([^/]+\.a)\(", owner)
                component = archive.group(1) if archive else owner.rsplit("/", 1)[-1]
                components[component] += stop - start
                category = section['category']
                component_categories[component][category] += stop - start
                objects[(component, owner)][category] += stop - start
                covered += stop - start
            end = max(end, stop)
    signature = [{k: s[k] for k in ("name", "address", "size")} for s in sections if s["kind"] == "sram"]
    signature.sort(key=lambda x: (x["address"], x["name"]))
    needed = ["_iram_start", "_iram_end", "_data_start", "_heap_start"]
    if any(name not in values for name in needed):
        raise ValueError("ELF sem os limites de RAM esperados do linker ESP32-S3")
    return {"v": 1, "ready": True, "version": version,
            "layout_signature": signature,
            "layout_hash": hashlib.sha256(json.dumps(signature, sort_keys=True).encode()).hexdigest(),
            "iram_reserved": values["_iram_end"] - values["_iram_start"],
            "dram_reserved": values["_heap_start"] - values["_data_start"],
            "static_section_bytes": sum(s["size"] for s in sections if s["kind"] == "sram"),
            "map_attributed_bytes": covered, "map_overlap_bytes": overlap,
            "components": [{"name": name, "size": size,
                            **{key: component_categories[name][key] for key in ('code', 'data', 'bss')}}
                           for name, size in sorted(components.items(), key=lambda item: -item[1])],
            "objects": sorted([{"component": component, "name": owner, "size": sum(categories.values()),
                                **{key: categories[key] for key in ('code', 'data', 'bss')}}
                               for (component, owner), categories in objects.items()], key=lambda item: (-item['size'], item['name'])),
            "sections": sections, "symbols": sorted(symbols, key=lambda s: (-s["size"], s["name"])),
            "note": "Symbols can alias or overlap. Do not sum symbol sizes. Section gaps and alignment remain unattributed. Flash code/data are excluded."}


def load_report(build: Path, version: str) -> dict:
    description = json.loads((build / "project_description.json").read_text(encoding="utf-8"))
    name = description.get("project_elf") or description["project_name"] + ".elf"
    elf = (build / Path(name).name).resolve()
    if elf.parent != build.resolve() or not elf.is_file():
        raise ValueError("ELF do projeto nao encontrado no diretorio de build")
    return inventory(elf.read_bytes(), elf.with_suffix(".map").read_text(encoding="utf-8", errors="replace"), version)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", required=True, type=Path)
    parser.add_argument("--sdkconfig")
    parser.add_argument("--rebuild", action="store_true")
    args = parser.parse_args()
    firmware = Path(__file__).resolve().parent.parent / "firmware"
    build = args.build_dir.resolve()
    version = (firmware / "version.txt").read_text(encoding="utf-8").strip()
    report = load_report(build, version)
    output = build / "jr_memory_build.json"
    for _ in range(3):
        output.write_text(json.dumps(report, ensure_ascii=True, separators=(",", ":")) + "\n", encoding="utf-8")
        if not args.rebuild:
            print("[OK] Inventario RAM gerado. Recompile para inclui-lo no firmware.")
            return 0
        if not args.sdkconfig:
            raise ValueError("--rebuild requer --sdkconfig")
        idf = Path(os.environ["IDF_PATH"]) / "tools" / "idf.py"
        subprocess.run([sys.executable, str(idf), "-B", str(build), "-D", "SDKCONFIG=" + args.sdkconfig, "build"], cwd=firmware, check=True)
        current = load_report(build, version)
        # JSON is placed in flash. Require both RAM layout and attributed objects
        # to remain unchanged after the relink; never embed a stale RAM report.
        if current == report:
            print("[OK] Inventario RAM do ELF/map incorporado e validado no firmware.")
            return 0
        report = current
    raise RuntimeError("Layout RAM nao convergiu apos incorporar inventario; gravacao interrompida")


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, KeyError, RuntimeError, subprocess.CalledProcessError, struct.error) as error:
        print("[ERRO] Inventario RAM: " + str(error), file=sys.stderr)
        raise SystemExit(1)
