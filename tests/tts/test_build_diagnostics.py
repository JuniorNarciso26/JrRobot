"""Exercise ELF32 symbol binding and metadata filtering without an ESP toolchain."""
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("tts_build_diagnostics", ROOT / "tools/tts_build_diagnostics.py")
DIAG = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(DIAG)


def fixture_elf(path):
    strings = b"\0vtts_dot_s8s8\0vtts_par_run\0"
    symbols = bytes(16)
    symbols += struct.pack("<IIIBBH", 1, 0x42000000, 64, 2 << 4 | 2, 0, 0xfff1)
    symbols += struct.pack("<IIIBBH", strings.index(b"vtts_par_run"), 0x42000100, 128, 1 << 4 | 2, 0, 0xfff1)
    sym_offset = 52 + len(strings)
    sh_offset = sym_offset + len(symbols)
    ident = b"\x7fELF\x01\x01\x01" + bytes(9)
    header = struct.pack("<16sHHIIIIIHHHHHH", ident, 2, 94, 1, 0, 0, sh_offset, 0, 52, 0, 0, 40, 3, 0)
    sections = bytes(40)
    sections += struct.pack("<IIIIIIIIII", 0, 3, 0, 0, 52, len(strings), 0, 0, 1, 0)
    sections += struct.pack("<IIIIIIIIII", 0, 2, 0, 0, sym_offset, len(symbols), 1, 0, 4, 16)
    path.write_bytes(header + strings + symbols + sections)


class BuildDiagnosticsTest(unittest.TestCase):
    def test_symbol_bindings_and_missing_symbols(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "test.elf"
            fixture_elf(path)
            data = DIAG.elf_symbols(path)
            self.assertTrue(data["is_xtensa"])
            self.assertEqual(data["symbols"]["vtts_dot_s8s8"]["binding"], "WEAK")
            self.assertEqual(data["symbols"]["vtts_par_run"]["binding"], "GLOBAL")
            self.assertIsNone(data["symbols"]["vtts_dots_s8s8"])

    def test_report_reads_generated_config_and_filters_secrets(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            build = root / "firmware/b-develop-vtts"
            (build / "config").mkdir(parents=True)
            fixture_elf(build / "jrbot_v2.elf")
            cfg = build.parent / "sdkconfig.develop_vtts"
            cfg.write_text('CONFIG_ESP32S3_DATA_CACHE_64KB=y\nCONFIG_WIFI_PASSWORD="never-export"\n')
            (build / "config/sdkconfig.h").write_text('#define CONFIG_FREERTOS_HZ 1000\n#define SECRET_TOKEN "never-export"\n')
            (build / "compile_commands.json").write_text(json.dumps([
                {"file": "src/vtts_synth.c", "command": 'cc -Os -O2 -DVTTS_BLOCK=64 -DSECRET="never-export" src/vtts_synth.c'},
                {"file": "other.c", "command": "never-export"}]))
            with patch.object(DIAG, "ROOT", root):
                report = DIAG.build_report(build)
            serialized = json.dumps(report)
            self.assertNotIn("never-export", serialized)
            self.assertEqual(report["sdkconfig"]["CONFIG_ESP32S3_DATA_CACHE_64KB"], "y")
            self.assertEqual(report["generated_config"]["CONFIG_FREERTOS_HZ"], "1000")
            self.assertEqual(report["compile_units"][0]["optimization_flags"], ["-Os", "-O2"])
            self.assertEqual(report["compile_units"][0]["vtts_definitions"], ["VTTS_BLOCK=64"])

    def test_invalid_binary_and_missing_build_fail_without_creating_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = root / "bad.elf"
            path.write_bytes(b"not an ELF")
            with self.assertRaises(ValueError):
                DIAG.elf_symbols(path)
            with self.assertRaises(ValueError):
                DIAG.build_report(root / "missing")
            self.assertEqual(list(root.iterdir()), [path])


if __name__ == "__main__":
    unittest.main()
