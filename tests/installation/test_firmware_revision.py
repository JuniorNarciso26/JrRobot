"""Offline checks for firmware version stamping and stable main/develop installer."""
from __future__ import annotations

import importlib.util
from pathlib import Path
import re
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "jr_stamp", ROOT / "tools" / "stamp_firmware_revision.py"
)
assert spec and spec.loader
stamp_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(stamp_module)


class FirmwareRevisionTests(unittest.TestCase):
    def test_version_label_is_consistent_and_fits_esp_app_descriptor(self):
        version = (ROOT / "firmware" / "version.txt").read_text().strip()
        config = (ROOT / "firmware" / "core" / "jr_config.h").read_text()
        match = re.search(r'^#define JR_APP_VERSION "([^"]+)"', config, re.MULTILINE)
        self.assertIsNotNone(match)
        self.assertEqual(version, match.group(1))
        self.assertLessEqual(len(version.encode("ascii")), 31)

    def test_stamp_is_stable_and_replaces_only_when_commit_changes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            header = root / "firmware" / "core" / "jr_firmware_revision_local.h"
            with mock.patch.object(stamp_module.subprocess, "check_output", return_value="a" * 40):
                self.assertEqual("aaaaaaaa", stamp_module.stamp(root))
                original = header.stat().st_mtime_ns
                self.assertEqual("aaaaaaaa", stamp_module.stamp(root))
                self.assertEqual(original, header.stat().st_mtime_ns)
            with mock.patch.object(stamp_module.subprocess, "check_output", return_value="b" * 40):
                self.assertEqual("bbbbbbbb", stamp_module.stamp(root))
            self.assertIn('#define JR_FIRMWARE_GIT_COMMIT "bbbbbbbb"', header.read_text())

    def test_develop_installer_flashes_without_confirmation(self):
        installer = (ROOT / "INSTALAR.bat").read_text()
        self.assertIn("python tools\\stamp_firmware_revision.py", installer)
        self.assertIn("-p \"%JR_FLASH_PORT%\" flash", installer)
        self.assertNotIn("CONFIRM_TTS_FLASH", installer)
        self.assertIn("[GRAVADO] ESP-IDF confirmou", installer)

    def test_main_and_develop_use_same_vtts_https_idf6_profile(self):
        installer = (ROOT / "INSTALAR.bat").read_text()
        self.assertIn('if /i "%CURRENT_BRANCH%"=="main" set "JRBOT_VTTS_BRANCH=1"', installer)
        self.assertIn('if /i "%CURRENT_BRANCH%"=="develop" set "JRBOT_VTTS_BRANCH=1"', installer)
        self.assertIn('set "SDKCONFIG_FILE=sdkconfig.develop_vtts"', installer)
        self.assertIn('set "JRBOT_TTS_CMAKE_ARGS=-D JRBOT_ENABLE_VTTS=ON"', installer)
        self.assertIn('set "BUILD_DIR=b-main-vtts"', installer)
        self.assertIn('python tools\\prepare_https_develop.py', installer)
        self.assertIn('python tools\\prepare_vtts_develop.py', installer)
        self.assertIn('python tools\\stamp_firmware_revision.py', installer)
        self.assertNotIn("Digite GRAVAR", installer)
        deps = (ROOT / "firmware/main/idf_component.yml").read_text()
        self.assertIn('>=6.1.0,<6.2.0', deps)

    def test_runtime_reports_commit_in_boot_and_status(self):
        boot = (ROOT / "firmware" / "main" / "main.c").read_text()
        commands = (ROOT / "firmware" / "core" / "jr_commands.c").read_text()
        self.assertIn("BOOT %s build_sp=%s git=%s", boot)
        self.assertIn("JR_STATUS protocol=2 version=%s build_sp=%s git=%s", commands)
        self.assertIn("JR_OK version=%s build_sp=%s git=%s", commands)


if __name__ == "__main__":
    unittest.main()
