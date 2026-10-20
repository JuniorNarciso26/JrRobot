"""Host checks for the production mDNS wrapper and cached SDK configuration."""
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
MOCK = r'''
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_INVALID_STATE 2
#define MALLOC_CAP_INTERNAL 1
#define MALLOC_CAP_8BIT 2
#define MALLOC_CAP_DMA 4
#define MALLOC_CAP_SPIRAM 8
const char *esp_err_to_name(esp_err_t);
size_t heap_caps_get_free_size(unsigned);
size_t heap_caps_get_largest_free_block(unsigned);
typedef struct { int unused; } esp_netif_t;
esp_netif_t *esp_netif_get_handle_from_ifkey(const char *);
esp_err_t esp_netif_get_hostname(esp_netif_t *,const char **);
esp_err_t mdns_init(void);
void mdns_free(void);
esp_err_t mdns_hostname_set(const char *);
esp_err_t mdns_service_add(const char *,const char *,const char *,uint16_t,void *,size_t);
#define MDNS_EVENT_ENABLE_IP4 2
#define MDNS_EVENT_ANNOUNCE_IP4 8
esp_err_t mdns_netif_action(esp_netif_t *,int);
'''
CONFIG = '''
#define CONFIG_MDNS_MEMORY_ALLOC_SPIRAM 1
#define CONFIG_MDNS_TASK_CREATE_FROM_INTERNAL 1
#define CONFIG_MDNS_TASK_STACK_SIZE 4096
#define CONFIG_MDNS_MAX_INTERFACES 1
#define CONFIG_MDNS_MAX_SERVICES 1
#define CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE 2304
#define CONFIG_LWIP_MAX_SOCKETS 16
'''
HARNESS = r'''
#include <assert.h>
#include <string.h>
#include "jr_mdns.c"
static bool connected,missing_netif,empty_hostname;
static jr_mode_t mode;
static int fail_stage;
static unsigned inits,frees,services,actions;
static esp_netif_t sta;
static const char *name="jrbot";
const char *esp_err_to_name(esp_err_t err) { return err==ESP_OK?"ESP_OK":"TEST_ERROR"; }
size_t heap_caps_get_free_size(unsigned caps) { assert(caps);return 80000; }
size_t heap_caps_get_largest_free_block(unsigned caps) { assert(caps);return 32000; }
bool jr_wifi_is_connected(void) { return connected; }
void jr_wifi_get_status(jr_wifi_status_t *s) { memset(s,0,sizeof(*s));strcpy(s->ip,"192.168.0.123"); }
jr_mode_t jr_mode_get(void) { return mode; }
esp_netif_t *esp_netif_get_handle_from_ifkey(const char *key) { assert(!strcmp(key,"WIFI_STA_DEF"));return missing_netif?NULL:&sta; }
esp_err_t esp_netif_get_hostname(esp_netif_t *p,const char **out) { assert(p==&sta);*out=empty_hostname?"":name;return ESP_OK; }
esp_err_t mdns_init(void) { inits++;return fail_stage==1?ESP_FAIL:ESP_OK; }
void mdns_free(void) { frees++; }
esp_err_t mdns_hostname_set(const char *hostname) { assert(!strcmp(hostname,name));return fail_stage==2?ESP_FAIL:ESP_OK; }
esp_err_t mdns_service_add(const char *instance,const char *type,const char *proto,uint16_t port,void *txt,size_t n) {
    assert(!strcmp(instance,"JrBot")&&!strcmp(type,"_https")&&!strcmp(proto,"_tcp")&&port==443&&!txt&&!n);
    services++;return fail_stage==3?ESP_FAIL:ESP_OK;
}
esp_err_t mdns_netif_action(esp_netif_t *p,int action) {
    assert(p==&sta);actions++;
    assert(action==(actions==1?MDNS_EVENT_ENABLE_IP4:MDNS_EVENT_ANNOUNCE_IP4));
    return fail_stage==(actions==1?4:5)?ESP_FAIL:ESP_OK;
}
static void reset(void) {
    attempted=false;connected=true;missing_netif=empty_hostname=false;mode=JR_MODE_IDLE;
    inits=frees=services=actions=0;fail_stage=0;name="jrbot";
}
int main(void) {
    reset();connected=false;jr_mdns_poll();assert(!inits&&!attempted);
    connected=true;mode=JR_MODE_LIVE;jr_mdns_poll();assert(!inits&&!attempted);
    mode=JR_MODE_IDLE;jr_mdns_poll();assert(inits==1&&services==1&&actions==2&&!frees);
    jr_mdns_poll();assert(inits==1);connected=false;jr_mdns_poll();connected=true;jr_mdns_poll();assert(inits==1);
    for(int i=1;i<=5;i++) {
        reset();fail_stage=i;jr_mdns_poll();assert(inits==1&&frees==(i>1?1U:0U));
        jr_mdns_poll();assert(inits==1);
    }
    reset();missing_netif=true;jr_mdns_poll();assert(!inits);
    reset();empty_hostname=true;jr_mdns_poll();assert(!inits);
    reset();name="my-robot";jr_mdns_poll();assert(inits==1&&actions==2);
    return 0;
}
'''


class MdnsTests(unittest.TestCase):
    def test_initialization_gates_announcements_and_cleanup(self):
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            (directory / "mock.h").write_text(MOCK)
            (directory / "sdkconfig.h").write_text(CONFIG)
            for header in ["esp_err.h", "esp_netif.h", "esp_heap_caps.h", "mdns.h"]:
                (directory / header).write_text('#include "mock.h"\n')
            (directory / "main.c").write_text(HARNESS)
            executable = directory / "test"
            command = ["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-fsanitize=address,undefined",
                       "-I", str(directory), "-I", str(ROOT / "firmware/core"),
                       str(directory / "main.c"), "-o", str(executable)]
            subprocess.run(command, check=True)
            result = subprocess.run([str(executable)], capture_output=True, text=True,
                                    env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("url=https://jrbot.local", result.stdout)
            self.assertIn("url=https://my-robot.local", result.stdout)
            for option in ["CONFIG_MDNS_MEMORY_ALLOC_SPIRAM", "CONFIG_MDNS_TASK_CREATE_FROM_INTERNAL"]:
                (directory / "sdkconfig.h").write_text(CONFIG.replace(f"#define {option} 1", f"#define {option} 0"))
                bad = subprocess.run(command, capture_output=True, text=True)
                self.assertNotEqual(bad.returncode, 0)

    def test_sdkconfig_migration_preserves_live_settings(self):
        spec = importlib.util.spec_from_file_location("mdns_settings", ROOT / "tools/ensure_mdns_sdkconfig.py")
        helper = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(helper)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "sdkconfig"
            helper.apply(path)
            self.assertFalse(path.exists())
            keep = "CONFIG_LWIP_MAX_SOCKETS=16\nCONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE=2304\nCONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC=y\n"
            path.write_text(keep + "CONFIG_MDNS_MEMORY_ALLOC_INTERNAL=y\n# CONFIG_MDNS_MEMORY_ALLOC_SPIRAM is not set\n")
            helper.apply(path)
            first = path.read_text()
            self.assertTrue(first.startswith(keep))
            self.assertIn("CONFIG_MDNS_MEMORY_ALLOC_SPIRAM=y", first)
            self.assertNotIn("CONFIG_MDNS_MEMORY_ALLOC_INTERNAL=y", first)
            helper.apply(path)
            self.assertEqual(first, path.read_text())
            defaults = (ROOT / "firmware/sdkconfig.defaults").read_text()
            for key, value in helper.OPTIONS.items():
                line = f"# {key} is not set" if value == "n" else f"{key}={value}"
                self.assertIn(line, defaults)


if __name__ == "__main__":
    unittest.main()
