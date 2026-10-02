import base64
import hashlib
import json
from pathlib import Path
import struct
import sys
import threading
import unittest
from unittest.mock import patch
import urllib.error
import urllib.request

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import solana_skill as skill
import app
import docs_panel  # Exercise the same wrapper chain used by run_panel.py.

PAYLOAD = (Path(__file__).resolve().parents[3] / "docs/HACKATHON_DEVLOG/assets/day3/recovered-skill.json").read_bytes()


def fixture():
    data = (hashlib.sha256(b"account:Skill").digest()[:8] + skill.AUTHORITY_BYTES + b"\x01"
            + hashlib.sha256(PAYLOAD).digest() + struct.pack("<I", len(PAYLOAD)) + PAYLOAD)
    data += bytes(589 - len(data))
    return {"context": {"slot": 123}, "value": {"owner": skill.PROGRAM, "executable": False,
            "data": [base64.b64encode(data).decode(), "base64"]}}


class SolanaSkillTests(unittest.TestCase):
    def test_exact_bytes_and_finalized_request(self):
        calls = []
        def rpc(method, params, timeout):
            calls.append((method, params))
            return skill.GENESIS if method == "getGenesisHash" else fixture()
        proof = skill.load_skill(rpc)
        self.assertEqual(proof["payload_text"].encode(), PAYLOAD)
        self.assertEqual(proof["payload_hash"], skill.EXPECTED_HASH)
        self.assertEqual(calls[1][1], [skill.PDA, {"encoding": "base64", "commitment": "finalized"}])

    def test_wrong_network_stops_before_account_read(self):
        calls = []
        def rpc(method, params, timeout):
            calls.append(method)
            return "wrong-network"
        with self.assertRaises(ValueError):
            skill.load_skill(rpc)
        self.assertEqual(calls, ["getGenesisHash"])

    def test_corrupted_account_and_metadata_are_rejected(self):
        for offset in (0, 8, 40, 41, 73, 77):
            with self.subTest(offset=offset):
                result = fixture()
                data = bytearray(base64.b64decode(result["value"]["data"][0]))
                data[offset] ^= 255
                result["value"]["data"][0] = base64.b64encode(data).decode()
                with self.assertRaises(ValueError):
                    skill.decode_account(result)
        for value in (None, {"owner": "other"}, {"owner": skill.PROGRAM, "executable": True}):
            with self.assertRaises(ValueError):
                skill.decode_account({"value": value})
        result = fixture()
        result["value"]["data"] = ["not-base64!", "base64"]
        with self.assertRaises(ValueError):
            skill.decode_account(result)

    def test_timeout_propagates_without_local_fallback(self):
        def rpc(*args):
            raise TimeoutError("offline")
        with patch("builtins.open", side_effect=AssertionError("No local files allowed")):
            with self.assertRaises(TimeoutError):
                skill.load_skill(rpc)

    def test_http_route_success_and_fail_closed(self):
        server = app.ThreadingHTTPServer(("127.0.0.1", 0), app.Handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        url = f"http://127.0.0.1:{server.server_port}/jrskill/skill-devnet"
        try:
            with urllib.request.urlopen(f"http://127.0.0.1:{server.server_port}/") as response:
                html = response.read().decode()
                self.assertIn(app.APP_VERSION, html)
                self.assertIn("Executar Skill da Devnet", html)
                self.assertIn("Executar Skill JSON local", html)
            status_url = url.replace("skill-devnet", "devnet-status")
            with patch.object(skill, "check_connection", return_value={"available": True, "cluster": "devnet"}):
                with urllib.request.urlopen(status_url) as response:
                    self.assertTrue(json.load(response)["available"])
            with patch.object(skill, "check_connection", side_effect=TimeoutError("offline")):
                with urllib.request.urlopen(status_url) as response:
                    self.assertFalse(json.load(response)["available"])
            with patch.object(skill, "load_skill", return_value=skill.decode_account(fixture())):
                with urllib.request.urlopen(url) as response:
                    proof = json.load(response)
                    self.assertEqual(proof["payload_text"].encode(), PAYLOAD)
                    self.assertEqual(response.headers["Cache-Control"], "no-store")
            with patch.object(skill, "load_skill", side_effect=TimeoutError("offline")):
                with self.assertRaises(urllib.error.HTTPError) as error:
                    urllib.request.urlopen(url)
                self.assertEqual(error.exception.code, 502)
        finally:
            server.shutdown()
            server.server_close()
            thread.join()


    def test_connection_probe_rejects_wrong_network(self):
        with self.assertRaises(ValueError):
            skill.check_connection(lambda *args: "wrong-network")
        self.assertEqual(skill.check_connection(lambda *args: skill.GENESIS), {"available": True, "cluster": "devnet"})


if __name__ == "__main__":
    unittest.main()
