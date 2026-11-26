"""Integration checks against the real, pinned upstream clone (no board required)."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("prepare_tflite", ROOT / "tools/prepare_tflite_yes_no.py")
prep = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prep)
SOURCE = ROOT / ".tflite-yes-no/upstream"


@unittest.skipUnless((SOURCE / ".git").exists(), "Run tools/prepare_tflite_yes_no.py first")
class OfficialExamplePreparation(unittest.TestCase):
    def test_real_example_preserves_models_features_and_repeated_build_state(self):
        with tempfile.TemporaryDirectory() as tmp:
            project = Path(tmp) / "project"
            prep.prepare(SOURCE, project)
            for name in ("model.cc", "micro_features_generator.cc",
                         "audio_preprocessor_int8_model_data.h", "micro_model_settings.h"):
                self.assertEqual((project / "main" / name).read_bytes(),
                                 (SOURCE / "examples/micro_speech/main" / name).read_bytes())
            retained = [project / "sdkconfig", project / "dependencies.lock", project / "build/cache"]
            for path in retained:
                path.parent.mkdir(exist_ok=True)
                path.write_text("existing build state")
            before = (project / "main/audio_provider.cc").read_bytes()
            prep.prepare(SOURCE, project)
            self.assertEqual(before, (project / "main/audio_provider.cc").read_bytes())
            for path in retained:
                self.assertEqual(path.read_text(), "existing build state")
            self.assertEqual(prep.git(SOURCE, "status", "--porcelain"), "")

    def test_modified_upstream_is_rejected_without_overwriting(self):
        with tempfile.TemporaryDirectory() as tmp:
            clone = Path(tmp) / "upstream"
            subprocess.run(["git", "clone", "--shared", str(SOURCE), str(clone)],
                           check=True, capture_output=True)
            modified = clone / "CMakeLists.txt"
            original = modified.read_bytes() + b"\n# user's local change\n"
            modified.write_bytes(original)
            with self.assertRaises(RuntimeError):
                prep.prepare(clone, Path(tmp) / "project")
            self.assertEqual(modified.read_bytes(), original)
            self.assertFalse((Path(tmp) / "project").exists())


if __name__ == "__main__":
    unittest.main()
