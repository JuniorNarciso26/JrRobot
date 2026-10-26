"""Offline unit tests, no physical ESP32 required."""
import importlib.util
from pathlib import Path
import subprocess
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / "tools" / "ler_mac_esp32.py"
spec = importlib.util.spec_from_file_location("ler_mac_esp32", SCRIPT)
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)

class TestMacReader(unittest.TestCase):
    def test_parse_and_device_id(self):
        mac = reader.parse_mac("Connected to ESP32-S3\nMAC: 34:85:18:ab:cd:ef (Espressif)\n")
        self.assertEqual(mac, "34:85:18:AB:CD:EF")
        self.assertEqual(reader.device_id(mac), "JRBOT-348518ABCDEF")

    def test_reject_bad_values(self):
        for value in ["No MAC found", "MAC: 00:00:00:00:00:00", "MAC: FF:FF:FF:FF:FF:FF", "MAC: 01:22:33:44:55:66"]:
            with self.subTest(value=value), self.assertRaises(reader.MacReadError):
                reader.parse_mac(value)

    def test_v5_esptool(self):
        calls = []
        def run(args, **kwargs):
            calls.append(args)
            if "--help" in args:
                return subprocess.CompletedProcess(args, 0, "read-mac", "")
            return subprocess.CompletedProcess(args, 0, "MAC: 34:85:18:AB:CD:EF", "")
        self.assertEqual(reader.read_mac("COM6", runner=run), "34:85:18:AB:CD:EF")
        self.assertEqual(calls[1][-4:], ["esp32s3", "--port", "COM6", "read-mac"])

    def test_v4_esptool(self):
        calls = []
        def run(args, **kwargs):
            calls.append(args)
            if "--help" in args:
                return subprocess.CompletedProcess(args, 0, "read_mac", "")
            return subprocess.CompletedProcess(args, 0, "MAC: 34:85:18:AB:CD:EF", "")
        self.assertEqual(reader.read_mac("COM7", runner=run), "34:85:18:AB:CD:EF")
        self.assertEqual(calls[1][-1], "read_mac")

    def test_failures(self):
        def run(args, **kwargs):
            if "--help" in args:
                return subprocess.CompletedProcess(args, 0, "read-mac", "")
            return subprocess.CompletedProcess(args, 2, "", "Failed to connect")
        with self.assertRaisesRegex(reader.MacReadError, "Failed to connect"):
            reader.read_mac("COM9", runner=run)

    def test_timeout(self):
        def run(args, **kwargs):
            raise subprocess.TimeoutExpired(args, 15)
        with self.assertRaises(reader.MacReadError):
            reader.read_mac("COM9", runner=run)

if __name__ == "__main__":
    unittest.main()
