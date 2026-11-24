"""Run real TTS buffered synthesis/player code with bounded pthread/I2S host doubles.
Run: python -m unittest discover -s tests/tts
Full ESP-IDF build and timing/audio validation still require ESP32-S3.
"""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class BufferedTest(unittest.TestCase):
    def test_buffered_lifecycle(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = str(Path(tmp) / "stream")
            subprocess.run([
                "cc", "-std=gnu11", "-Wall", "-Wextra", "-Werror",
                "-ffunction-sections", "-fdata-sections", "-Itests/tts/host",
                "-Ifirmware/module_tts", "-Ifirmware/core", "-Ifirmware/module_brain",
                "tests/tts/buffered_host.c", "-Wl,--gc-sections", "-lm", "-pthread", "-o", binary,
            ], cwd=ROOT, check=True)
            subprocess.run([binary], check=True, timeout=15)
