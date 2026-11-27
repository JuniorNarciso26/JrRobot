"""Compile the real SoftAP module with driver/NVS failure injection on the host.
Not an ESP-IDF build or RF/WebRTC validation.
"""
from pathlib import Path
import os
import runpy
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
BASE = runpy.run_path(str(ROOT / 'tests/wifi_profiles/test_profiles.py'))['MOCK']
MOCK = BASE.replace('} sta; } wifi_config_t;', '''} sta;
struct { unsigned char ssid[32],password[64]; unsigned ssid_len,authmode,max_connection,channel;
struct { bool capable,required; } pmf_cfg; } ap; } wifi_config_t;''') + r'''
#define ESP_NETIF_DEFAULT_WIFI_AP() {0}
#define WIFI_MODE_AP 55
#define WIFI_IF_AP 56
#define ESP_MAC_WIFI_SOFTAP 57
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
esp_err_t esp_read_mac(uint8_t *,int);
esp_err_t esp_netif_attach_wifi_ap(esp_netif_t *);
esp_err_t esp_wifi_set_default_wifi_ap_handlers(void);
esp_err_t nvs_set_str(nvs_handle_t,const char *,const char *);
esp_err_t nvs_set_u8(nvs_handle_t,const char *,uint8_t);
'''
HARNESS = r'''
#include <assert.h>
#include "jr_wifi_direct.c"
static uint8_t stored_enabled, pending_enabled;
static char stored_password[65], pending_password[65];
static bool writing, fail_commit, fail_nvs, netif_live, initialized, started;
static int fail_step, step;
static unsigned starts, stops, deinits;
static wifi_config_t applied;
static esp_netif_t netif;
static int result(void) { return ++step == fail_step ? ESP_FAIL : ESP_OK; }
esp_err_t nvs_open(const char *name,int mode,nvs_handle_t *out) {
    assert(!strcmp(name,"jr_direct"));
    if(fail_nvs) return ESP_FAIL;
    *out=1;writing=mode==NVS_READWRITE;
    pending_enabled=stored_enabled;strcpy(pending_password,stored_password);return ESP_OK;
}
void nvs_close(nvs_handle_t h) { assert(h==1);writing=false; }
esp_err_t nvs_get_u8(nvs_handle_t h,const char *key,uint8_t *value) {
    assert(h==1&&!strcmp(key,"enabled"));*value=stored_enabled;return ESP_OK;
}
esp_err_t nvs_set_u8(nvs_handle_t h,const char *key,uint8_t value) {
    assert(h==1&&writing&&!strcmp(key,"enabled"));pending_enabled=value;return ESP_OK;
}
esp_err_t nvs_get_str(nvs_handle_t h,const char *key,char *value,size_t *size) {
    assert(h==1&&!strcmp(key,"password"));
    size_t n=strlen(stored_password)+1;
    if(n>*size)return ESP_ERR_INVALID_SIZE;
    memcpy(value,stored_password,n);*size=n;return ESP_OK;
}
esp_err_t nvs_set_str(nvs_handle_t h,const char *key,const char *value) {
    assert(h==1&&writing&&!strcmp(key,"password"));strcpy(pending_password,value);return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t h) {
    assert(h==1&&writing);if(fail_commit)return ESP_FAIL;
    stored_enabled=pending_enabled;strcpy(stored_password,pending_password);return ESP_OK;
}
esp_err_t esp_netif_init(void) { return result(); }
esp_err_t esp_event_loop_create_default(void) { return result(); }
esp_netif_t *esp_netif_new(const esp_netif_config_t *c) {
    (void)c;if(result()!=ESP_OK)return NULL;assert(!netif_live);netif_live=true;return &netif;
}
esp_err_t esp_netif_attach_wifi_ap(esp_netif_t *p) { assert(p==&netif);return result(); }
esp_err_t esp_wifi_set_default_wifi_ap_handlers(void) { return result(); }
void esp_netif_destroy_default_wifi(esp_netif_t *p) { assert(p==&netif&&netif_live);netif_live=false; }
esp_err_t esp_wifi_init(const wifi_init_config_t *c) {
    (void)c;int err=result();if(!err)initialized=true;return err;
}
esp_err_t esp_wifi_set_storage(int mode) { assert(mode==WIFI_STORAGE_RAM);return result(); }
esp_err_t esp_read_mac(uint8_t *mac,int type) {
    assert(type==ESP_MAC_WIFI_SOFTAP);memcpy(mac,"\x01\x02\x03\xAB\xCD\xEF",6);return result();
}
esp_err_t esp_wifi_set_mode(int mode) { assert(mode==WIFI_MODE_AP);return result(); }
esp_err_t esp_wifi_set_config(int iface,const wifi_config_t *c) {
    assert(iface==WIFI_IF_AP);applied=*c;return result();
}
esp_err_t esp_wifi_start(void) {
    assert(initialized);int err=result();if(!err){started=true;starts++;}return err;
}
esp_err_t esp_wifi_stop(void) { assert(started);started=false;stops++;return ESP_OK; }
esp_err_t esp_wifi_deinit(void) { assert(initialized&&!started);initialized=false;deinits++;return ESP_OK; }
int main(void) {
    assert(!jr_wifi_direct_enabled());
    assert(jr_wifi_direct_start()==ESP_ERR_INVALID_STATE);
    assert(jr_wifi_direct_configure(true,NULL)==ESP_ERR_INVALID_ARG);
    assert(jr_wifi_direct_configure(true,"short")==ESP_ERR_INVALID_ARG);
    assert(jr_wifi_direct_configure(true,"abcdefgh\n")==ESP_ERR_INVALID_ARG);
    assert(jr_wifi_direct_configure(true,"senha-com-acent\xc3\xb3")==ESP_ERR_INVALID_ARG);
    char maxpass[65];memset(maxpass,'x',64);maxpass[64]=0;
    assert(jr_wifi_direct_configure(true,maxpass)==ESP_ERR_INVALID_ARG);
    maxpass[63]=0;assert(jr_wifi_direct_configure(true,maxpass)==ESP_OK);
    fail_commit=true;
    assert(jr_wifi_direct_configure(false,NULL)==ESP_FAIL);
    assert(jr_wifi_direct_enabled());fail_commit=false;
    assert(jr_wifi_direct_configure(true,"secret123")==ESP_OK);
    for(int i=1;i<=11;i++) {
        step=0;fail_step=i;
        assert(jr_wifi_direct_start()!=ESP_OK);
        assert(!jr_wifi_direct_active()&&!netif_live&&!initialized&&!started);
    }
    fail_step=0;step=0;
    assert(jr_wifi_direct_start()==ESP_OK);
    assert(jr_wifi_direct_active()&&netif_live&&started);
    assert(!strcmp(jr_wifi_direct_ssid(),"JrBot-ABCDEF"));
    assert(!memcmp(applied.ap.ssid,"JrBot-ABCDEF",12));
    assert(!strcmp((char *)applied.ap.password,"secret123"));
    assert(applied.ap.authmode==WIFI_AUTH_WPA2_PSK&&applied.ap.max_connection==2);
    assert(applied.ap.ssid_len==12&&applied.ap.channel==1);
    assert(jr_wifi_direct_start()==ESP_OK&&starts==1); /* idempotent */
    assert(jr_wifi_direct_configure(false,NULL)==ESP_OK);
    assert(!jr_wifi_direct_enabled()&&jr_wifi_direct_active()); /* reboot applies */
    jr_wifi_direct_stop();
    assert(!netif_live&&!initialized&&!started&&stops==1&&deinits);
    jr_wifi_direct_stop(); /* safe repeated stop */
    stored_enabled=1;stored_password[0]=0;
    assert(jr_wifi_direct_start()==ESP_ERR_INVALID_STATE&&!jr_wifi_direct_active());
    fail_nvs=true;assert(!jr_wifi_direct_enabled());
    assert(jr_wifi_direct_configure(true,"secret123")==ESP_FAIL);
    return 0;
}
'''

class DirectTests(unittest.TestCase):
    def test_softap_lifecycle_credentials_and_failure_cleanup(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            (folder/'mock.h').write_text(MOCK)
            for header in ('esp_err.h','esp_event.h','esp_log.h','esp_mac.h',
                           'esp_netif.h','esp_wifi.h','esp_wifi_default.h','nvs.h'):
                (folder/header).write_text('#include "mock.h"\n')
            (folder/'main.c').write_text(HARNESS)
            exe=folder/'direct'
            subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-I',str(folder),
                            '-I',str(ROOT/'firmware/core'),str(folder/'main.c'),
                            '-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True,env={**os.environ,"ASAN_OPTIONS":"detect_leaks=0"})

if __name__ == '__main__':
    unittest.main()
