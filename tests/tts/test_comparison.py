"""Test the real HTTPS benchmark orchestration with a fake Voice API, not audio."""
from contextlib import redirect_stdout
import importlib.util
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("tts_benchmark", ROOT / "tools/tts_benchmark.py")
BENCH = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BENCH)


class FakeVoice:
    def __init__(self):
        self.posts = []
        self.status = dict(BENCH.SETTINGS, enabled=False, available=True, busy=False,
                           i2s_owner="none", utterances=0, last_error="ESP_OK")
        self.queued = False
        self.fail_send = False

    def request(self, payload=None):
        if payload:
            self.posts.append(payload)
            if payload["action"] == "configure":
                self.status.update({k: v for k, v in payload.items() if k != "action"})
                return {"ok": True, "saved": True}
            if self.fail_send:
                raise OSError("response lost after possible delivery")
            self.queued = True
            return {"ok": True, "queued": True}
        if self.queued:
            self.status.update(utterances=1, last_first_audio_ms=1000, last_audio_ms=17900,
                               last_synth_ms=6000, last_rtf=0.335, last_total_ms=24000)
        return self.status.copy()


class ComparisonTest(unittest.TestCase):
    def run_fake(self, client, root, enter=lambda: ""):
        with patch.object(BENCH, "ROOT", root), patch.object(BENCH, "VoiceClient", return_value=client), patch("builtins.input", enter), redirect_stdout(io.StringIO()):
            return BENCH.run("https://jrbot.local", True)

    def test_same_canonical_phrase_and_one_send_after_enter(self):
        client = FakeVoice()
        with tempfile.TemporaryDirectory() as tmp:
            def enter():
                self.assertEqual([p["action"] for p in client.posts], ["configure"])
                return ""
            path = self.run_fake(client, Path(tmp), enter)
            log = path.read_text()
            self.assertIn(BENCH.PHRASE_SHA256, log)
            self.assertIn("JR_TTS_BENCH_DONE", log)
            self.assertEqual(client.posts, [{"action": "configure", **BENCH.SETTINGS},
                                           {"action": "speak", "text": BENCH.standard_phrase()}])

    def test_no_automatic_retry_on_uncertain_delivery(self):
        client = FakeVoice()
        client.fail_send = True
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(OSError):
                self.run_fake(client, Path(tmp))
            self.assertEqual(sum(p["action"] == "speak" for p in client.posts), 1)
            log = next(Path(tmp).rglob("*.txt")).read_text()
            self.assertIn("DELIVERY_MAY_HAVE_OCCURRED=1", log)

    def test_active_audio_blocks_before_configuration(self):
        client = FakeVoice()
        client.status["i2s_owner"] = "legacy_voice"
        with tempfile.TemporaryDirectory() as tmp, self.assertRaisesRegex(RuntimeError, "Autonomo"):
            self.run_fake(client, Path(tmp))
        self.assertEqual(client.posts, [])

    def test_local_https_only(self):
        for url in ["http://jrbot.local", "https://example.com", "https://8.8.8.8", "https://user:secret@jrbot.local", "https://jrbot.local/voice", "https://jrbot.local?x=1"]:
            with self.subTest(url=url), self.assertRaises(ValueError):
                BENCH.local_origin(url)
        self.assertEqual(BENCH.local_origin("https://192.168.0.123/"), "https://192.168.0.123")


if __name__ == "__main__":
    unittest.main()
