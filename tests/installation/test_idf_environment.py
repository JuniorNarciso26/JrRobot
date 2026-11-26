"""Offline SDK/venv validation; no Windows environment or package installs."""
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('idf_env', ROOT/'tools/prepare_idf_environment.py')
idf_env = importlib.util.module_from_spec(spec)
spec.loader.exec_module(idf_env)


class IDFEnvironmentTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)/'sdk'
        self.venv = Path(self.tmp.name)/'venv'
        self.venv.mkdir()
        for name, text in {
            'tools/cmake/version.cmake': 'set(IDF_VERSION_MAJOR 6)\nset(IDF_VERSION_MINOR 1)\nset(IDF_VERSION_PATCH 1)\n',
            'components/esp_common/include/esp_idf_version.h': '#define ESP_IDF_VERSION_MAJOR 6\n#define ESP_IDF_VERSION_MINOR 1\n#define ESP_IDF_VERSION_PATCH 1\n',
            'tools/idf.py': '', 'tools/idf_tools.py': ''
        }.items():
            p = self.root/name
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text(text)

    def prepare(self, runner):
        with mock.patch.object(sys, 'prefix', str(self.venv)), mock.patch.object(idf_env.subprocess, 'run', runner):
            idf_env.prepare(self.root, self.venv, '6.1')

    def test_create_missing_marker_only_after_official_dependency_check(self):
        marker = self.venv/'idf_version.txt'
        def check(command, **kwargs):
            self.assertFalse(marker.exists())
            self.assertEqual(command[-1], 'check-python-dependencies')
            self.assertTrue(kwargs['check'])
            self.assertEqual(kwargs['env']['IDF_PATH'], str(self.root))
        self.prepare(check)
        self.assertEqual(marker.read_text(), '6.1')
        stamp = marker.stat().st_mtime_ns
        self.prepare(mock.Mock())
        self.assertEqual(marker.stat().st_mtime_ns, stamp)

    def test_failed_dependencies_do_not_create_marker(self):
        with self.assertRaises(subprocess.CalledProcessError):
            self.prepare(mock.Mock(side_effect=subprocess.CalledProcessError(1, 'check')))
        self.assertFalse((self.venv/'idf_version.txt').exists())

    def test_wrong_marker_is_preserved_and_not_checked_or_overwritten(self):
        marker = self.venv/'idf_version.txt'
        marker.write_text('5.5')
        runner = mock.Mock()
        with self.assertRaises(ValueError): self.prepare(runner)
        runner.assert_not_called()
        self.assertEqual(marker.read_text(), '5.5')

    def test_reject_wrong_sdk_family_and_inconsistent_metadata(self):
        with self.assertRaises(ValueError): idf_env.source_version(self.root, '5.5')
        (self.root/'version.txt').write_text('v5.5.1')
        with self.assertRaises(ValueError): idf_env.source_version(self.root, '6.1')
        (self.root/'version.txt').unlink()
        path = self.root/'tools/cmake/version.cmake'
        path.write_text(path.read_text().replace('PATCH 1', 'PATCH 2'))
        with self.assertRaises(ValueError): idf_env.source_version(self.root, '6.1')

    def test_reject_different_running_interpreter(self):
        runner = mock.Mock()
        with mock.patch.object(sys, 'prefix', str(self.root)), mock.patch.object(idf_env.subprocess, 'run', runner):
            with self.assertRaises(ValueError): idf_env.prepare(self.root, self.venv, '6.1')
        runner.assert_not_called()

    def test_version_cli_without_git_tags_and_real_failure_exit_code(self):
        cmd = [sys.executable, str(ROOT/'tools/prepare_idf_environment.py'), '--idf-root', str(self.root), '--version-only']
        result = subprocess.run(cmd, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('6.1.1', result.stdout)
        self.assertNotIn('fatal', result.stderr)
        result = subprocess.run(cmd+['--expected', '5.5'], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)

    def test_installer_runs_preparation_before_export_and_has_no_version_pipeline(self):
        bat = (ROOT/'INSTALAR.bat').read_text()
        self.assertLess(bat.index('--export "%IDF_EXPORT%"'), bat.index('call "%IDF_EXPORT%"'))
        self.assertIn('--idf-root "%IDF_PATH%" --expected 6.1 --version-only', bat)
        self.assertNotIn('idf.py" --version |', bat)


if __name__ == '__main__': unittest.main()
