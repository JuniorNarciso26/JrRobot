"""Host regression for the exact Wi-Fi recovery guard in production provisioning code.

Run: python -m unittest discover -s tests/wifi_recovery -v
Compiles extracted C functions with host mocks. This is not a hardware/ESP-IDF test.
"""
import re
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "firmware" / "core" / "jr_wifi_provisioning.c"
MAIN = ROOT / "firmware" / "main" / "main.c"
HEADER = ROOT / "firmware" / "core" / "jr_wifi_provisioning.h"


def extract_function(source: str, name: str) -> str:
    match = re.search(r"\\b(?:bool|void)\\s+" + re.escape(name) + r"\\s*\\([^)]*\\)\\s*\\{", source)
    if not match:
        raise AssertionError("Missing production function: " + name)
    depth = 0
    for i in range(match.end() - 1, len(source)):
        if source[i] == "{":
            depth += 1
        elif source[i] == "}":
            depth -= 1
            if depth == 0:
                return source[match.start():i + 1]
    raise AssertionError("Unclosed function: " + name)


class RecoveryGuardTests(unittest.TestCase):
    def test_guard_survives_software_restart_and_clears_on_success(self):
        source = SOURCE.read_text(encoding="utf-8")
        names = (
            "jr_wifi_provisioning_boot_requested",
            "jr_wifi_provisioning_auto_restart_once",
            "jr_wifi_provisioning_recovery_connected",
        )
        functions = "\\n\\n".join(extract_function(source, name) for name in names)
        defines = "\\n".join(line for line in source.splitlines()
                            if line.startswith("#define JR_SETUP_MAGIC ") or
                            line.startswith("#define JR_WIFI_RECOVERY_MAGIC "))
        harness = r"""
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#define ESP_RST_SW 3
#define ESP_LOGW(...) ((void)0)
typedef int esp_reset_reason_t;
static esp_reset_reason_t reason;
static bool configured = true;
static esp_reset_reason_t esp_reset_reason(void) { return reason; }
static bool jr_wifi_is_configured(void) { return configured; }
static uint32_t setup_request, setup_request_inverse;
static uint32_t recovery_restart_marker, recovery_restart_marker_inverse;
""" + defines + "\\n" + functions + r"""
int main(void) {
    reason = 0; /* cold boot */
    assert(!jr_wifi_provisioning_boot_requested());
    assert(jr_wifi_provisioning_auto_restart_once());
    assert(!jr_wifi_provisioning_auto_restart_once()); /* no reboot loop */

    reason = ESP_RST_SW; /* automatic reboot */
    assert(!jr_wifi_provisioning_boot_requested());
    assert(!jr_wifi_provisioning_auto_restart_once()); /* go to captive portal */

    jr_wifi_provisioning_recovery_connected(); /* DHCP success */
    assert(jr_wifi_provisioning_auto_restart_once()); /* rearm next outage */

    setup_request = JR_SETUP_MAGIC;
    setup_request_inverse = ~JR_SETUP_MAGIC;
    assert(jr_wifi_provisioning_boot_requested()); /* BOOT button still works */
    assert(!jr_wifi_provisioning_boot_requested());

    reason = 0; /* power cycle resets the reboot budget */
    assert(!jr_wifi_provisioning_boot_requested());
    assert(jr_wifi_provisioning_auto_restart_once());

    configured = false; /* unconfigured still starts captive portal */
    assert(jr_wifi_provisioning_boot_requested());
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as temp:
            folder = Path(temp)
            src = folder / "recovery.c"
            exe = folder / "recovery"
            src.write_text(harness, encoding="utf-8")
            subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                            str(src), "-o", str(exe)], check=True, capture_output=True, text=True)
            subprocess.run([str(exe)], check=True, capture_output=True, text=True)

    def test_wiring_in_main_and_public_header(self):
        main = MAIN.read_text(encoding="utf-8")
        header = HEADER.read_text(encoding="utf-8")
        self.assertIn("jr_wifi_provisioning_auto_restart_once()", main)
        self.assertIn("jr_wifi_provisioning_recovery_connected()", main)
        self.assertIn("esp_restart();", main)
        for fn in ("jr_wifi_provisioning_auto_restart_once",
                   "jr_wifi_provisioning_recovery_connected"):
            self.assertIn(fn, header)


if __name__ == "__main__":
    unittest.main()
