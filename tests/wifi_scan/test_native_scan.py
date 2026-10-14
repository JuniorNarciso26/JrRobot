"""Compile the production scan with a simulated driver; no ESP-IDF required."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
HEADERS = {
    "esp_err.h": """#pragma once
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_INVALID_SIZE 1
const char *esp_err_to_name(esp_err_t);
""",
    "esp_heap_caps.h": """#pragma once
#include <stddef.h>
#define MALLOC_CAP_INTERNAL 1
#define MALLOC_CAP_8BIT 2
size_t heap_caps_get_free_size(unsigned);
size_t heap_caps_get_largest_free_block(unsigned);
""",
    "esp_wifi.h": """#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#define WIFI_SCAN_TYPE_ACTIVE 0
typedef struct { int scan_type; struct { struct { unsigned min,max; } active; } scan_time; } wifi_scan_config_t;
typedef struct { uint8_t ssid[33]; int8_t rssi; unsigned authmode; uint8_t primary; } wifi_ap_record_t;
esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *,bool);
esp_err_t esp_wifi_scan_get_ap_num(uint16_t *);
esp_err_t esp_wifi_scan_get_ap_record(wifi_ap_record_t *);
esp_err_t esp_wifi_clear_ap_list(void);
""",
}
HARNESS = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "jr_wifi.h"
#include "jr_mode_manager.h"
static jr_wifi_status_t status;
static jr_mode_t mode;
static unsigned starts,clears,reads,remaining,total,hidden;
static int start_error,num_error,record_error,cleanup_error;
void jr_wifi_get_status(jr_wifi_status_t *s) { *s=status; }
jr_mode_t jr_mode_get(void) { return mode; }
size_t heap_caps_get_free_size(unsigned caps) { (void)caps; return 75000; }
size_t heap_caps_get_largest_free_block(unsigned caps) { (void)caps; return 31000; }
const char *esp_err_to_name(esp_err_t e) { return e==ESP_OK?"ESP_OK":"TEST_ERROR"; }
esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *c,bool block) {
    assert(c->scan_type==WIFI_SCAN_TYPE_ACTIVE && c->scan_time.active.max<=120 && block);
    starts++; if(start_error)return start_error; remaining=total; return ESP_OK;
}
esp_err_t esp_wifi_scan_get_ap_num(uint16_t *n) { *n=total; return num_error; }
esp_err_t esp_wifi_scan_get_ap_record(wifi_ap_record_t *a) {
    if(record_error)return record_error;
    assert(remaining>0); remaining--; reads++;
    memset(a,0,sizeof(*a));
    if(reads>hidden)memcpy(a->ssid,"\"%|&<>0123456789012345678901234567",32);
    a->rssi=-48; a->authmode=4; a->primary=1; return ESP_OK;
}
esp_err_t esp_wifi_clear_ap_list(void) { clears++; remaining=0; return cleanup_error; }
static void reset(void) {
    status=(jr_wifi_status_t){.connected=true,.network_ready=true}; mode=JR_MODE_IDLE;
    starts=clears=reads=remaining=hidden=0; total=2;
    start_error=num_error=record_error=cleanup_error=0;
}
int main(void) {
    char reply[2048];
    reset();status.connected=false;assert(!jr_wifi_scan(reply,sizeof(reply)));assert(!starts);
    reset();status.pending_restart=true;assert(!jr_wifi_scan(reply,sizeof(reply)));assert(!starts);
    reset();mode=JR_MODE_LIVE;assert(!jr_wifi_scan(reply,sizeof(reply)));assert(!starts);
    reset();mode=JR_MODE_AUTONOMOUS;assert(!jr_wifi_scan(reply,sizeof(reply)));assert(!starts);
    reset();assert(jr_wifi_scan(reply,sizeof(reply)));assert(clears==1 && !remaining && reads==2);puts(reply);
    reset();total=0;assert(jr_wifi_scan(reply,sizeof(reply)));assert(clears==1 && !reads);puts(reply);
    reset();total=20;assert(jr_wifi_scan(reply,sizeof(reply)));assert(clears==1 && !remaining && reads==8);puts(reply);
    reset();total=4;hidden=4;assert(jr_wifi_scan(reply,sizeof(reply)));assert(clears==1 && !remaining);puts(reply);
    reset();start_error=ESP_FAIL;assert(!jr_wifi_scan(reply,sizeof(reply)));assert(!clears);
    reset();num_error=ESP_FAIL;assert(!jr_wifi_scan(reply,sizeof(reply)));assert(clears==1 && !remaining);
    reset();record_error=ESP_FAIL;assert(!jr_wifi_scan(reply,sizeof(reply)));assert(clears==1 && !remaining);
    reset();cleanup_error=ESP_FAIL;assert(!jr_wifi_scan(reply,sizeof(reply)));assert(clears==1);
    reset();assert(!jr_wifi_scan(reply,30));assert(clears==1 && !remaining);
    reset();assert(!jr_wifi_scan(reply,1));assert(clears==1 && !remaining);
    reset();assert(!jr_wifi_scan(NULL,0));assert(!starts);
    return 0;
}
'''


class NativeScanTests(unittest.TestCase):
    def test_cleanup_limits_and_guard(self):
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            for name, text in HEADERS.items():
                (directory / name).write_text(text, encoding="utf-8")
            (directory / "main.c").write_text(HARNESS, encoding="utf-8")
            executable = directory / "test"
            subprocess.run([
                "gcc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                "-fsanitize=address,undefined", "-I", str(directory),
                "-I", str(ROOT / "firmware/core"), str(directory / "main.c"),
                str(ROOT / "firmware/core/jr_wifi_scan.c"), "-o", str(executable),
            ], check=True)
            # The fake driver checks ownership; LeakSanitizer needs /proc access.
            env = {**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}
            result = subprocess.run([str(executable)], check=True, capture_output=True, text=True, env=env)
        payloads = [json.loads(line.removeprefix("JR_WIFI_SCAN "))
                    for line in result.stdout.splitlines() if line.startswith("JR_WIFI_SCAN ")]
        self.assertEqual([len(p["networks"]) for p in payloads], [2, 0, 8, 0])
        self.assertTrue(payloads[2]["limited"])
        self.assertEqual(len(payloads[0]["networks"][0]["ssid_pct"]), 96)


if __name__ == "__main__":
    unittest.main()
