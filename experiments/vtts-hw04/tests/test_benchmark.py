"""Host tests for the fixed baseline sender; does not simulate ESP32 audio."""
from contextlib import redirect_stdout
import hashlib
import importlib.util
import io
from pathlib import Path
import tempfile
import types
import unittest
from unittest.mock import patch

BASE = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("tts_benchmark", BASE / "benchmark.py")
BENCH = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BENCH)


class FakeSerial:
    def __init__(self, **kwargs):
        self.pending = bytearray()
        self.sent = []
        self.closed = False
        self.identity = True

    def open(self):
        pass

    def reset_input_buffer(self):
        self.pending.clear()

    def flush(self):
        pass

    def close(self):
        self.closed = True

    def write(self, data):
        self.sent.append(data)
        command = data.decode("utf-8").strip()
        if command == "model" or command == "model s":
            response = "model S (token set pt-en, lang pt)" if self.identity else "unknown firmware"
        elif command == "speed 1":
            response = "speed 1.00"
        elif command == "expr 1":
            response = "expr 1.00 (prosody model: 1)"
        elif command == "gain 6":
            response = "gain 6.0 dB"
        elif command == "prof":
            response = "PROF tokens 0.100s"
        elif command in BENCH.SETTINGS:
            response = command
        else:
            response = "SAID model=S audio=20.00s synth=6.000s RTF=0.300"
        self.pending.extend((response + "\nDONE\n").encode())
        return len(data)

    def read(self, size):
        data = self.pending[:size]
        del self.pending[:size]
        return bytes(data)


class BenchmarkTest(unittest.TestCase):
    def test_settings_and_exact_phrase_sent_once_after_enter(self):
        device = FakeSerial()
        phrase = BENCH.standard_phrase()
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "frase_padrao.txt").write_text(phrase + "\n", encoding="utf-8")
            def enter():
                self.assertEqual(len(device.sent), 1 + len(BENCH.SETTINGS) + 1)
                self.assertNotIn((phrase + "\n").encode(), device.sent)
                return ""
            with patch.object(BENCH, "BASE", root), patch.dict("sys.modules", {"serial": types.SimpleNamespace(Serial=lambda **kw: device)}), patch("builtins.input", enter), redirect_stdout(io.StringIO()):
                BENCH.run("COM8")
            log = next((root / ".work/results").glob("*.txt")).read_text()
            self.assertEqual(device.sent.count((phrase + "\n").encode("utf-8")), 1)
            self.assertEqual(device.sent[1:1 + len(BENCH.SETTINGS)], [(x + "\n").encode() for x in BENCH.SETTINGS])
            self.assertIn(hashlib.sha256(phrase.encode()).hexdigest(), log)
            self.assertIn("JR_TTS_BENCH_DONE", log)
            self.assertTrue(device.closed)

    def test_wrong_firmware_never_receives_phrase(self):
        device = FakeSerial()
        device.identity = False
        phrase = BENCH.standard_phrase()
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "frase_padrao.txt").write_text(phrase, encoding="utf-8")
            with patch.object(BENCH, "BASE", root), patch.dict("sys.modules", {"serial": types.SimpleNamespace(Serial=lambda **kw: device)}), redirect_stdout(io.StringIO()), self.assertRaisesRegex(RuntimeError, "isolado nao identificado"):
                BENCH.run("COM8")
            self.assertEqual(device.sent, [b"model\n"])
            self.assertTrue(device.closed)

    def test_timeout_does_not_retry_and_device_errors_stop(self):
        device = FakeSerial()
        with self.assertRaisesRegex(RuntimeError, "Tempo esgotado"):
            BENCH.read_done(device, lambda line: None, 0)
        self.assertEqual(device.sent, [])
        device.pending.extend(b"ERROR no PSRAM\nDONE\n")
        with self.assertRaisesRegex(RuntimeError, "no PSRAM"):
            BENCH.read_done(device, lambda line: None, 1)

    def test_watchdog_aborts_and_fragments_are_not_lost(self):
        device = FakeSerial()
        device.pending.extend("português\nDONE\n".encode())
        self.assertEqual(BENCH.read_done(device, lambda line: None, 1), ["português"])
        device.pending.extend(b"E task_wdt: IDLE0\n")
        with self.assertRaisesRegex(RuntimeError, "Watchdog"):
            BENCH.read_done(device, lambda line: None, 1)


if __name__ == "__main__":
    unittest.main()
