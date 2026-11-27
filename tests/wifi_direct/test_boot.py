"""Run the production app_main boot branches with simulated time and services."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
MOCK=r'''
#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
#define ESP_LOGW(...) ((void)0)
#define pdMS_TO_TICKS(x) (x)
int64_t esp_timer_get_time(void);
void esp_restart(void);
void vTaskDelay(unsigned);
'''
HARNESS=r'''
#include <assert.h>
#include <setjmp.h>
#include "main.c"
static jmp_buf done;
static bool enabled,configured,forced,active,connected,fail_direct;
static unsigned portals,setups,stations,directs,restarts,loops;
static int64_t clock_us;
int64_t esp_timer_get_time(void) { return clock_us; }
void vTaskDelay(unsigned ms) { clock_us+=(uint64_t)ms*1000;if(++loops==2)longjmp(done,1); }
void esp_restart(void) { restarts++;longjmp(done,2); }
void jr_wifi_prepare(void) {}
void jr_tts_init(void) {}
esp_err_t jr_commands_init(void) { return ESP_OK; }
void jr_terminal_start(void) {}
void jr_usb_terminal_start(void) {}
esp_err_t jr_face_start(void) { return ESP_OK; }
void jr_face_step(uint32_t ms) { (void)ms; }
void jr_face_get_status(jr_face_status_t *s) { s->state=JR_OLED_READY; }
void jr_wifi_provisioning_button_init(void) {}
void jr_wifi_provisioning_button_poll(uint32_t ms) { (void)ms; }
bool jr_wifi_provisioning_boot_requested(void) { return forced||(!enabled&&!configured); }
esp_err_t jr_wifi_provisioning_start(void) { setups++;return ESP_OK; }
void jr_wifi_provisioning_poll(void) {}
void jr_wifi_provisioning_recovery_connected(void) {}
bool jr_wifi_provisioning_auto_restart_once(void) { return true; }
bool jr_wifi_direct_enabled(void) { return enabled; }
bool jr_wifi_direct_active(void) { return active; }
esp_err_t jr_wifi_direct_start(void) { directs++;active=!fail_direct;return active?ESP_OK:ESP_FAIL; }
void jr_wifi_start(void) { stations++; }
void jr_wifi_poll(void) {}
bool jr_wifi_network_ready(void) { return connected||active; }
bool jr_wifi_is_connected(void) { return connected; }
void jr_net_diag_watch(void) {}
void jr_net_diag_poll(uint32_t ms) { (void)ms; }
void jr_mdns_poll(void) {}
void jr_portal_start(void) { portals++; }
static void reset(void) {
    enabled=configured=forced=active=connected=fail_direct=false;
    portals=setups=stations=directs=restarts=loops=0;clock_us=0;
}
static void boot(void) { if(!setjmp(done))app_main(); }
int main(void) {
    reset();configured=connected=true;boot();assert(stations==1&&portals==1&&!directs&&!setups);
    reset();enabled=true;boot();assert(directs==1&&portals==1&&!stations&&!setups&&!restarts);
    reset();enabled=forced=true;boot();assert(setups==1&&!directs&&!portals&&!stations);
    reset();enabled=fail_direct=true;boot();assert(directs==1&&setups==1&&!stations&&!portals);
    reset();boot();assert(setups==1&&!directs&&!stations&&!portals);
    return 0;
}
'''
class BootTests(unittest.TestCase):
    def test_router_direct_forced_setup_and_direct_failure(self):
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp);(p/'mock.h').write_text(MOCK)
            for name in ('esp_err.h','esp_log.h','esp_timer.h','esp_system.h','freertos/FreeRTOS.h','freertos/task.h'):
                h=p/name;h.parent.mkdir(exist_ok=True);h.write_text('#include "mock.h"\n')
            (p/'boot.c').write_text(HARNESS)
            command=['gcc','-std=c11','-Wall','-Wextra','-Werror','-I',str(p)]
            for sub in ('main','core','module_face','module_portal','module_tts'):
                command+=['-I',str(ROOT/'firmware'/sub)]
            subprocess.run(command+[str(p/'boot.c'),'-o',str(p/'boot')],check=True)
            subprocess.run([str(p/'boot')],check=True)
