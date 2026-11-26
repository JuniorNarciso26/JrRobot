"""Real host archive-link regression and final ELF guard policy.
Portable stand-ins test symbol selection; they do not simulate S3 SIMD speed.
"""
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from check_tts_acceleration import require_acceleration


class AccelerationLinkTest(unittest.TestCase):
    def test_archive_weak_fallback_and_fixed_source_selection(self):
        cmake = (ROOT / "firmware/main/CMakeLists.txt").read_text()
        source_list = re.search(r"set\(JR_TTS_SOURCES\s*\n(.*?)\)", cmake, re.S).group(1)
        self.assertIn("ports/esp32s3/vtts_port_s3.c", source_list)
        self.assertIn("ports/freertos/vtts_par_freertos.c", source_list)
        self.assertNotIn("src/vtts_kernels_ref.c", source_list)
        files = {
            "fallback": '__attribute__((weak)) int dot(void){return 1;}\n__attribute__((weak)) void parallel(void(*fn)(void)){fn();}\n',
            "s3": 'int s3_kernel=0;\nint dot(void){return 24;}\n',
            "parallel": 'void parallel(void(*fn)(void)){fn();fn();}\n',
            "main": '#include <stdio.h>\nint dot(void);void parallel(void(*fn)(void));\nstatic int calls;static void fn(void){calls++;}\nint main(void){parallel(fn);printf("%d %d\\n",dot(),calls);}\n',
        }
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for name, text in files.items():
                (root / (name + ".c")).write_text(text)
                subprocess.run(["cc", "-O2", "-c", str(root / (name + ".c")), "-o", str(root / (name + ".o"))], check=True)
            def link_and_run(name, members):
                archive = root / (name + ".a")
                subprocess.run(["ar", "rcs", str(archive), *[str(root / (member + ".o")) for member in members]], check=True)
                binary = root / name
                subprocess.run(["cc", str(root / "main.o"), str(archive), "-o", str(binary)], check=True)
                return subprocess.check_output([str(binary)], text=True).strip()
            # With the weak object selected first, available strong objects are skipped.
            self.assertEqual(link_and_run("old", ["fallback", "s3", "parallel"]), "1 1")
            # The supported S3 build omits the weak defaults, forcing real implementations.
            self.assertEqual(link_and_run("fixed", ["s3", "parallel"]), "24 2")
            # A missing port must fail linkage instead of accepting generic execution.
            archive = root / "missing.a"
            subprocess.run(["ar", "rcs", str(archive), str(root / "parallel.o")], check=True)
            result = subprocess.run(["cc", str(root / "main.o"), str(archive), "-o", str(root / "missing")], capture_output=True)
            self.assertNotEqual(result.returncode, 0)

    def test_elf_guard_rejects_weak_and_missing_required_implementations(self):
        good = {"is_xtensa": True, "symbols": {
            name: {"binding": "GLOBAL", "type": 2, "bytes": 64}
            for name in ("vtts_dot_s8s8", "vtts_dots_s8s8", "vtts_par_run")}}
        good["symbols"]["vtts_s3_kernel"] = {"binding": "GLOBAL", "type": 1, "bytes": 4}
        require_acceleration(good)
        for name in good["symbols"]:
            for bad in [None, {"binding": "WEAK", "type": 2, "bytes": 14},
                        {"binding": "LOCAL", "type": 2, "bytes": 64},
                        {"binding": "GLOBAL", "type": 2 if name == "vtts_s3_kernel" else 1, "bytes": 64},
                        {"binding": "GLOBAL", "type": 2, "bytes": 0}]:
                symbols = dict(good["symbols"], **{name: bad})
                if name == "vtts_dot_s8s8" and bad is None:
                    require_acceleration({"is_xtensa": True, "symbols": symbols})
                    continue
                with self.subTest(name=name, value=bad), self.assertRaises(ValueError):
                    require_acceleration({"is_xtensa": True, "symbols": symbols})
        with self.assertRaises(ValueError):
            require_acceleration({**good, "is_xtensa": False})


if __name__ == "__main__":
    unittest.main()
