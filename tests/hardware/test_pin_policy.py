#!/usr/bin/env python3
"""C11 compile checks for the fixed HW04 final map.

This is not an ESP-IDF build or a physical hardware test. It only compiles
small translation units with static assertions. On developer machines it can
use a host compiler (GCC/Clang); on Windows installations it may also use the
ESP-IDF Xtensa cross compiler because the generated object is never executed.
"""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def _resolve_executable(value: str | None) -> str | None:
    if not value:
        return None
    found = shutil.which(value)
    if found:
        return found
    candidate = Path(value).expanduser()
    if candidate.is_file():
        return str(candidate)
    return None


def find_c_compiler() -> str | None:
    explicit = _resolve_executable(os.environ.get("CC"))
    if explicit:
        return explicit

    for name in (
        "gcc",
        "clang",
        "cc",
        "xtensa-esp32s3-elf-gcc",
        "xtensa-esp-elf-gcc",
    ):
        found = shutil.which(name)
        if found:
            return found

    tools_root = Path(
        os.environ.get("IDF_TOOLS_PATH", str(Path.home() / ".espressif"))
    ).expanduser()
    if tools_root.is_dir():
        wanted = (
            "xtensa-esp32s3-elf-gcc.exe",
            "xtensa-esp-elf-gcc.exe",
            "xtensa-esp32s3-elf-gcc",
            "xtensa-esp-elf-gcc",
        )
        for binary in wanted:
            matches = sorted(tools_root.rglob(binary), reverse=True)
            if matches:
                return str(matches[0])
    return None


def main():
    cc = find_c_compiler()
    if not cc:
        raise SystemExit(
            "Nenhum compilador C11 encontrado. Abra pelo terminal ESP-IDF 5.5.x "
            "ou configure CC para GCC/Clang."
        )

    print(f"C11 compiler: {cc}")
    subprocess.run(
        [sys.executable, str(ROOT / "tools/generate_pinmap.py"), "--check"],
        check=True,
    )

    cases = []
    for name, defines in [
        ("final_default", {}),
        (
            "stale_old_config",
            {"CONFIG_JR_HEADLESS_DIAGNOSTIC": 1, "CONFIG_JR_AUDIO_HW04_CONFIRMED": 0},
        ),
    ]:
        with tempfile.TemporaryDirectory(prefix="jrbot_hw04_") as tmp:
            d = Path(tmp)
            (d / "sdkconfig.h").write_text(
                "\n".join(f"#define {k} {v}" for k, v in defines.items()) + "\n"
            )
            (d / "test.c").write_text(
                '#include "jr_board.h"\n'
                '_Static_assert(JR_AUDIO_BCLK_GPIO==21, "bclk");\n'
                '_Static_assert(JR_AUDIO_LRC_GPIO==47, "ws");\n'
                '_Static_assert(JR_AUDIO_DIN_GPIO==42, "dout");\n'
                '_Static_assert(JR_MIC_SD_GPIO==41, "mic_sd");\n'
                '_Static_assert(JR_AUDIO_ENABLED==1, "audio");\n'
                '_Static_assert(JR_MIC_ENABLED==1, "mic");\n'
                '_Static_assert(JR_CAMERA_ENABLED==1, "camera");\n'
                '_Static_assert(JR_OLED_ENABLED==1, "oled");\n'
                "int main(void){return 0;}\n"
            )
            cmd = [
                cc,
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(d),
                "-I",
                str(ROOT / "firmware/core"),
                "-c",
                str(d / "test.c"),
                "-o",
                str(d / "out.o"),
            ]
            run = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
            ok = run.returncode == 0
            cases.append({"case": name, "passed": ok})
            print(("PASS " if ok else "FAIL ") + name)
            if not ok:
                print(run.stderr)

    result = {
        "scope": "C11 final pin-map compile checks only. No ESP-IDF build or physical test.",
        "compiler": cc,
        "passed": sum(x["passed"] for x in cases),
        "failed": sum(not x["passed"] for x in cases),
        "cases": cases,
    }
    (ROOT / "tests/hardware/results.json").write_text(
        json.dumps(result, indent=2) + "\n"
    )
    return 1 if result["failed"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
