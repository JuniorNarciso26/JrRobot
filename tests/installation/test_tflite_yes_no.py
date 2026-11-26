"""Integration checks against the real, pinned upstream clone (no board required)."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("prepare_tflite", ROOT / "tools/prepare_tflite_yes_no.py")
prep = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prep)
SOURCE = ROOT / ".tflite-yes-no" / prep.COMPONENT_DIR


@unittest.skipUnless((SOURCE / ".git").exists(), "Run tools/prepare_tflite_yes_no.py first")
class OfficialExamplePreparation(unittest.TestCase):
    def clone_source(self, destination):
        subprocess.run(["git", "clone", "--shared", str(SOURCE), str(destination)],
                       check=True, capture_output=True)
        prep.git(destination, "remote", "set-url", "origin", prep.UPSTREAM)

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
            clone = Path(tmp) / prep.COMPONENT_DIR
            self.clone_source(clone)
            modified = clone / "CMakeLists.txt"
            original = modified.read_bytes() + b"\n# user's local change\n"
            modified.write_bytes(original)
            with self.assertRaises(RuntimeError):
                prep.prepare(clone, Path(tmp) / "project")
            self.assertEqual(modified.read_bytes(), original)
            self.assertFalse((Path(tmp) / "project").exists())

    def test_legacy_installation_migrates_name_and_rebuilds_stale_requirements_once(self):
        with tempfile.TemporaryDirectory(prefix="jrbot test ") as tmp:
            work = Path(tmp)
            legacy = work / "upstream"
            self.clone_source(legacy)
            project = work / "project"
            (project / "main").mkdir(parents=True)
            manifest = project / "main/idf_component.yml"
            manifest.write_text('dependencies:\n  espressif/esp-tflite-micro:\n'
                                '    version: "*"\n    override_path: "../../upstream"\n')
            (project / "build").mkdir()
            (project / "build/CMakeCache.txt").write_text("old upstream component")
            (project / "dependencies.lock").write_text("old local component path")
            (project / "sdkconfig").write_text("existing board config")
            with patch.object(prep, "WORK", work):
                source = prep.ensure_source()
                self.assertEqual(source.name, "espressif__esp-tflite-micro")
                self.assertFalse(legacy.exists())
                prep.prepare(source, project)
            self.assertIn('override_path: "../../espressif__esp-tflite-micro"', manifest.read_text())
            self.assertFalse((project / "build").exists())
            self.assertFalse((project / "dependencies.lock").exists())
            self.assertEqual((project / "sdkconfig").read_text(), "existing board config")
            (project / "build").mkdir()
            (project / "build/CMakeCache.txt").write_text("new valid cache")
            (project / "dependencies.lock").write_text("new valid lock")
            prep.prepare(source, project)
            self.assertEqual((project / "build/CMakeCache.txt").read_text(), "new valid cache")
            self.assertEqual((project / "dependencies.lock").read_text(), "new valid lock")

    def test_noncanonical_component_name_rejected_before_creating_project(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "upstream"
            self.clone_source(source)
            with self.assertRaisesRegex(RuntimeError, "pasta da biblioteca"):
                prep.prepare(source, Path(tmp) / "project")
            self.assertFalse((Path(tmp) / "project").exists())

    def test_modified_legacy_checkout_is_not_renamed(self):
        with tempfile.TemporaryDirectory() as tmp:
            work = Path(tmp)
            legacy = work / "upstream"
            self.clone_source(legacy)
            changed = legacy / "CMakeLists.txt"
            changed.write_text(changed.read_text() + "\n# local modification\n")
            with patch.object(prep, "WORK", work):
                with self.assertRaises(RuntimeError):
                    prep.ensure_source()
            self.assertTrue(legacy.exists())
            self.assertFalse((work / prep.COMPONENT_DIR).exists())


if __name__ == "__main__":
    unittest.main()
