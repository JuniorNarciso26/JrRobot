"""Offline regression checks for the private develop HTTPS installer."""
from __future__ import annotations

import ast
import importlib.util
from pathlib import Path
import re
import tempfile
import unittest
from unittest import mock

HERE = Path(__file__).resolve()
PROJECT = HERE.parents[2]
spec = importlib.util.spec_from_file_location(
    "prepare_https_develop", PROJECT / "tools" / "prepare_https_develop.py"
)
assert spec is not None and spec.loader is not None
https = importlib.util.module_from_spec(spec)
spec.loader.exec_module(https)


class HTTPSPreparationTests(unittest.TestCase):
    def test_generated_key_and_certificate_match(self):
        try:
            from cryptography import x509
            from cryptography.hazmat.primitives import serialization
        except ImportError:
            self.skipTest("cryptography unavailable")
        header = https._generate()
        self.assertIn("#define JR_HTTPS_LOCAL_MATERIAL_AVAILABLE 1", header)

        def pem(name: str) -> bytes:
            match = re.search(
                r"static const char " + name + r"\[\] =\n((?:\"[^\"]+\"\n)+);",
                header,
            )
            self.assertIsNotNone(match)
            return "".join(
                ast.literal_eval(line)
                for line in match.group(1).splitlines()
            ).encode("ascii")

        cert = x509.load_pem_x509_certificate(pem("JR_HTTPS_CERT_PEM"))
        private = serialization.load_pem_private_key(
            pem("JR_HTTPS_KEY_PEM"), password=None
        )
        self.assertEqual(
            cert.public_key().public_numbers(), private.public_key().public_numbers()
        )
        self.assertIn("jrbot.local", cert.extensions.get_extension_for_class(
            x509.SubjectAlternativeName
        ).value.get_values_for_type(x509.DNSName))

    def test_reuses_existing_without_replacing_key(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "JrBot_Develop_Voz"
            target = root / https.RELATIVE_HEADER
            target.parent.mkdir(parents=True)
            content = (
                "#define JR_HTTPS_LOCAL_MATERIAL_AVAILABLE 1\n"
                "JR_HTTPS_CERT_PEM JR_HTTPS_KEY_PEM\n"
            )
            target.write_text(content)
            with mock.patch.object(https, "_generate", side_effect=AssertionError("should not generate")):
                self.assertEqual(0, https.prepare(root))
            self.assertEqual(content, target.read_text())

    def test_copies_valid_sibling_clone_and_keeps_it_private(self):
        with tempfile.TemporaryDirectory() as tmp:
            base = Path(tmp)
            root = base / "JrBot_Develop_Voz"
            old = base / "JrBot_Main" / https.RELATIVE_HEADER
            old.parent.mkdir(parents=True)
            old.write_text(
                "#define JR_HTTPS_LOCAL_MATERIAL_AVAILABLE 1\n"
                "JR_HTTPS_CERT_PEM JR_HTTPS_KEY_PEM\n"
            )
            with mock.patch.object(https, "_generate", side_effect=AssertionError("should reuse")):
                self.assertEqual(0, https.prepare(root))
            self.assertEqual(old.read_bytes(), (root / https.RELATIVE_HEADER).read_bytes())

    def test_rejects_invalid_existing_header_without_destroying_it(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "JrBot_Develop_Voz"
            target = root / https.RELATIVE_HEADER
            target.parent.mkdir(parents=True)
            target.write_text("invalid private data")
            self.assertEqual(2, https.prepare(root))
            self.assertEqual("invalid private data", target.read_text())


if __name__ == "__main__":
    unittest.main()
