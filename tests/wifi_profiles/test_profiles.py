"""Test production Wi-Fi settings/catalog against an in-memory NVS driver."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
MOCK = r'''
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
typedef int esp_err_t;
enum { ESP_OK=0,ESP_FAIL=-1,ESP_ERR_INVALID_ARG=1,ESP_ERR_INVALID_STATE=2,
       ESP_ERR_NO_MEM=3,ESP_ERR_NVS_NOT_FOUND=4,ESP_ERR_INVALID_SIZE=5,
       ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED=6,ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED=7 };
const char *esp_err_to_name(esp_err_t);
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(p) ((void)(p))
#define portEXIT_CRITICAL(p) ((void)(p))
typedef int nvs_handle_t;
#define NVS_READONLY 0
#define NVS_READWRITE 1
esp_err_t nvs_flash_init(void);
esp_err_t nvs_open(const char *,int,nvs_handle_t *);
esp_err_t nvs_get_str(nvs_handle_t,const char *,char *,size_t *);
esp_err_t nvs_get_blob(nvs_handle_t,const char *,void *,size_t *);
esp_err_t nvs_get_u8(nvs_handle_t,const char *,uint8_t *);
esp_err_t nvs_set_blob(nvs_handle_t,const char *,const void *,size_t);
esp_err_t nvs_erase_key(nvs_handle_t,const char *);
esp_err_t nvs_commit(nvs_handle_t);
void nvs_close(nvs_handle_t);
typedef const char *esp_event_base_t;
typedef void *esp_event_handler_instance_t;
extern const char *WIFI_EVENT,*IP_EVENT;
enum { WIFI_EVENT_STA_START,WIFI_EVENT_STA_DISCONNECTED,IP_EVENT_STA_GOT_IP,ESP_EVENT_ANY_ID,
       WIFI_EVENT_STA_BSS_RSSI_LOW,WIFI_EVENT_SCAN_DONE };
esp_err_t esp_event_loop_create_default(void);
esp_err_t esp_event_handler_instance_register(esp_event_base_t,int,void (*)(void *,esp_event_base_t,int32_t,void *),void *,esp_event_handler_instance_t *);
esp_err_t esp_event_handler_instance_unregister(esp_event_base_t,int,esp_event_handler_instance_t);
typedef struct { uint32_t addr; } ipv4_t;
typedef struct { ipv4_t ip,gw,netmask; } esp_netif_ip_info_t;
typedef struct { esp_netif_ip_info_t ip_info; } ip_event_got_ip_t;
#define IPSTR "%u.%u.%u.%u"
#define IP2STR(p) ((p)->addr&255),(((p)->addr>>8)&255),(((p)->addr>>16)&255),(((p)->addr>>24)&255)
typedef struct { int unused; } esp_netif_t,esp_netif_config_t;
#define ESP_NETIF_DEFAULT_WIFI_STA() {0}
typedef struct { struct { int type; union { ipv4_t ip4; } u_addr; } ip; } esp_netif_dns_info_t;
enum { ESP_IPADDR_TYPE_V4,ESP_NETIF_DNS_MAIN,ESP_NETIF_DNS_BACKUP };
esp_err_t esp_netif_init(void);
esp_netif_t *esp_netif_new(const esp_netif_config_t *);
esp_err_t esp_netif_attach_wifi_station(esp_netif_t *);
esp_err_t esp_netif_set_hostname(esp_netif_t *,const char *);
esp_err_t esp_netif_dhcpc_stop(esp_netif_t *);
esp_err_t esp_netif_dhcpc_start(esp_netif_t *);
esp_err_t esp_netif_set_ip_info(esp_netif_t *,const esp_netif_ip_info_t *);
esp_err_t esp_netif_set_dns_info(esp_netif_t *,int,const esp_netif_dns_info_t *);
void esp_netif_destroy_default_wifi(esp_netif_t *);
typedef struct { int unused; } wifi_init_config_t;
#define WIFI_INIT_CONFIG_DEFAULT() {0}
enum { WIFI_STORAGE_RAM,WIFI_AUTH_WPA2_PSK,WIFI_AUTH_OPEN,WIFI_MODE_STA,WIFI_IF_STA,
       WIFI_ALL_CHANNEL_SCAN=15,WIFI_CONNECT_AP_BY_SIGNAL=17,WIFI_SCAN_TYPE_ACTIVE };
typedef struct { struct { char ssid[32],password[64]; struct { int authmode; } threshold;
                int scan_method,sort_method; } sta; } wifi_config_t;
typedef struct { uint8_t *ssid; int scan_type; } wifi_scan_config_t;
typedef struct { uint8_t ssid[33],bssid[6]; int8_t rssi; uint8_t primary; int authmode; } wifi_ap_record_t;
typedef struct { uint8_t ssid[32],bssid[6]; uint8_t ssid_len,channel,authmode; } wifi_event_sta_connected_t;
esp_err_t esp_wifi_init(const wifi_init_config_t *);
esp_err_t esp_wifi_set_storage(int);
esp_err_t esp_wifi_set_mode(int);
esp_err_t esp_wifi_set_config(int,const wifi_config_t *);
esp_err_t esp_wifi_start(void);
esp_err_t esp_wifi_stop(void);
esp_err_t esp_wifi_connect(void);
esp_err_t esp_wifi_disconnect(void);
esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *,bool);
esp_err_t esp_wifi_scan_get_ap_records(uint16_t *,wifi_ap_record_t *);
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *);
esp_err_t esp_wifi_set_rssi_threshold(int32_t);
int64_t esp_timer_get_time(void);
esp_err_t esp_wifi_deinit(void);
esp_err_t esp_wifi_set_default_wifi_sta_handlers(void);
uint32_t ipaddr_addr(const char *);
'''
HARNESS = r'''
#include <assert.h>
#include "jr_wifi.c"
typedef struct { char key[16]; size_t size; unsigned char bytes[512]; } blob_t;
static blob_t blobs[8];
static bool fail_write,fail_commit;
static unsigned simulated_connects;
static char simulated_ssid[33];
static unsigned simulated_scans,simulated_disconnects,simulated_rssi_arms;
static int simulated_scan_method,simulated_sort_method;
static int simulated_mode=JR_MODE_IDLE;
static int64_t simulated_time_us;
static wifi_ap_record_t associated;
static wifi_ap_record_t scan_candidates[2];
static unsigned scan_candidate_count;
const char *WIFI_EVENT="WIFI_EVENT",*IP_EVENT="IP_EVENT";
static unsigned opens;
const char *esp_err_to_name(esp_err_t err) { return err==ESP_OK?"ESP_OK":"TEST_ERROR"; }
esp_err_t nvs_flash_init(void) { return ESP_OK; }
esp_err_t nvs_open(const char *ns,int mode,nvs_handle_t *out) { assert(!strcmp(ns,"jr_wifi")); (void)mode; *out=1; opens++; return ESP_OK; }
void nvs_close(nvs_handle_t h) { assert(h==1 && opens); opens--; }
esp_err_t nvs_get_str(nvs_handle_t h,const char *k,char *p,size_t *n) { (void)h;(void)k;(void)p;(void)n;return ESP_ERR_NVS_NOT_FOUND; }
esp_err_t nvs_get_u8(nvs_handle_t h,const char *k,uint8_t *p) { (void)h;(void)k;(void)p;return ESP_ERR_NVS_NOT_FOUND; }
static blob_t *find(const char *key) { for(unsigned i=0;i<8;i++)if(!strcmp(blobs[i].key,key))return &blobs[i];return NULL; }
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *size) {
    (void)h;blob_t *b=find(key);if(!b)return ESP_ERR_NVS_NOT_FOUND;
    if(*size<b->size)return ESP_ERR_INVALID_SIZE;
    memcpy(out,b->bytes,b->size);*size=b->size;return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *data,size_t size) {
    (void)h;if(fail_write)return ESP_FAIL;assert(size<=512);
    blob_t *b=find(key);if(!b)for(unsigned i=0;i<8;i++)if(!blobs[i].key[0]){b=&blobs[i];break;}
    assert(b);strcpy(b->key,key);b->size=size;memcpy(b->bytes,data,size);return ESP_OK;
}
esp_err_t nvs_erase_key(nvs_handle_t h,const char *key) { (void)h;blob_t *b=find(key);if(!b)return ESP_ERR_NVS_NOT_FOUND;memset(b,0,sizeof(*b));return ESP_OK; }
esp_err_t nvs_commit(nvs_handle_t h) { (void)h;return fail_commit?ESP_FAIL:ESP_OK; }
/* Minimal driver stubs for deterministic saved-network failover checks. */
esp_err_t esp_wifi_connect(void) { simulated_connects++; return ESP_OK; }
esp_err_t esp_wifi_disconnect(void) { simulated_disconnects++; return ESP_OK; }
esp_err_t esp_wifi_set_rssi_threshold(int32_t rssi) {
    assert(rssi==-70); simulated_rssi_arms++; return ESP_OK;
}
int64_t esp_timer_get_time(void) { return simulated_time_us; }
jr_mode_t jr_mode_get(void) { return (jr_mode_t)simulated_mode; }
esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *config,bool blocking) {
    assert(config && config->ssid && !blocking && !strcmp((const char *)config->ssid,active.ssid));
    simulated_scans++; return ESP_OK;
}
esp_err_t esp_wifi_scan_get_ap_records(uint16_t *size,wifi_ap_record_t *out) {
    unsigned n=scan_candidate_count<*size?scan_candidate_count:*size;
    memcpy(out,scan_candidates,n*sizeof(*out));*size=(uint16_t)n;return ESP_OK;
}
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *out) { *out=associated;return ESP_OK; }
esp_err_t esp_wifi_set_config(int iface,const wifi_config_t *c) {
    assert(iface==WIFI_IF_STA);
    snprintf(simulated_ssid,sizeof(simulated_ssid),"%s",c->sta.ssid);
    simulated_scan_method=c->sta.scan_method;
    simulated_sort_method=c->sta.sort_method;
    return ESP_OK;
}
esp_err_t esp_netif_set_hostname(esp_netif_t *dev,const char *name) {
    (void)dev; assert(name && name[0]); return ESP_OK;
}
esp_err_t esp_netif_dhcpc_start(esp_netif_t *dev) { (void)dev; return ESP_OK; }
esp_err_t esp_netif_dhcpc_stop(esp_netif_t *dev) { (void)dev; return ESP_OK; }
esp_err_t esp_netif_set_ip_info(esp_netif_t *dev,const esp_netif_ip_info_t *info) {
    (void)dev; (void)info; return ESP_OK;
}
esp_err_t esp_netif_set_dns_info(esp_netif_t *dev,int kind,const esp_netif_dns_info_t *info) {
    (void)dev; (void)kind; (void)info; return ESP_OK;
}
uint32_t ipaddr_addr(const char *ip) { (void)ip; return 0; }
static char reply[2048];
static bool save(const char *ssid,const char *pass) { return jr_wifi_profile_save(ssid,pass,"jrbot",false,"","","","","",reply,sizeof(reply)); }
static void check_selected(const char *ssid,const char *pass) {
    wifi_settings_t record;size_t size=sizeof(record);assert(nvs_get_blob(1,"cfg_v2",&record,&size)==ESP_OK);
    assert(!strcmp(record.ssid,ssid) && !strcmp(record.pass,pass));
}
static void reboot(void) { prepared=false;state=(jr_wifi_status_t){0};jr_wifi_prepare(); }
int main(void) {
    /* Simulate the approved single-network record before the upgrade. */
    wifi_settings_t original;defaults(&original);strcpy(original.ssid,"Original");strcpy(original.pass,"password123");
    assert(nvs_set_blob(1,"cfg_v2",&original,sizeof(original))==ESP_OK);reboot();
    assert(jr_wifi_profile_use(0,reply,sizeof(reply)));assert(!state.pending_restart);assert(strstr(reply,"pending_restart=0"));
    assert(jr_wifi_profiles_list(reply,sizeof(reply)));assert(strstr(reply,"\"id\":0"));assert(!strstr(reply,"password123"));puts(reply);
    assert(save("Second","password456"));check_selected("Original","password123");assert(!state.pending_restart);
    assert(jr_wifi_profiles_list(reply,sizeof(reply)));assert(strstr(reply,"\"id\":1"));puts(reply);
    assert(save("Third","password789"));assert(save("Open",""));
    assert(!save("Fifth","password123"));assert(strstr(reply,"profiles=full"));check_selected("Original","password123");
    assert(save("Second","newpassword"));
    assert(jr_wifi_profile_use(1,reply,sizeof(reply)));assert(state.pending_restart);check_selected("Second","newpassword");
    assert(jr_wifi_profile_delete(0,reply,sizeof(reply)));assert(strstr(reply,"setup_on_restart=0"));
    check_selected("Second","newpassword");
    reboot();assert(!strcmp(active.ssid,"Second"));assert(!strcmp(active.pass,"newpassword"));
    assert(jr_wifi_profile_use(1,reply,sizeof(reply)));assert(!state.pending_restart);assert(strstr(reply,"pending_restart=0"));
    state.pending_restart=true;
    assert(jr_wifi_profile_use(1,reply,sizeof(reply)));assert(!state.pending_restart);
    assert(!jr_wifi_profile_delete(0,reply,sizeof(reply)));check_selected("Second","newpassword");
    assert(save("Fifth","password123"));assert(jr_wifi_profiles_list(reply,sizeof(reply)));puts(reply);
    assert(!jr_wifi_profile_use(4,reply,sizeof(reply)));
    assert(!save("Invalid","short"));check_selected("Second","newpassword");
    assert(!jr_wifi_profiles_list(reply,20));assert(opens==0);
    fail_write=true;assert(!save("Sixth","password123"));fail_write=false;
    assert(jr_wifi_profile_delete(2,reply,sizeof(reply)));
    fail_write=true;assert(!save("Sixth","password123"));fail_write=false;check_selected("Second","newpassword");
    fail_commit=true;assert(!save("Sixth","password123"));fail_commit=false;check_selected("Second","newpassword");
    assert(jr_wifi_clear(reply,sizeof(reply)));reboot();assert(!jr_wifi_is_configured());
    assert(jr_wifi_profiles_list(reply,sizeof(reply)));puts(reply);
    assert(jr_wifi_profile_use(1,reply,sizeof(reply)));check_selected("Second","newpassword");
    assert(jr_wifi_configure("Second","anotherpassword","jrbot",false,"","","","","",reply,sizeof(reply)));
    check_selected("Second","anotherpassword");assert(opens==0);
    assert(state.pending_restart); /* Same SSID with a changed password still needs restart. */
    reboot();assert(!strcmp(active.ssid,"Second"));
    fail_write=true;assert(!jr_wifi_profile_delete(1,reply,sizeof(reply)));fail_write=false;
    check_selected("Second","anotherpassword");
    assert(jr_wifi_profile_delete(1,reply,sizeof(reply)));assert(strstr(reply,"setup_on_restart=1"));
    check_selected("","");assert(state.pending_restart && !state.configured);
    assert(!strcmp(active.ssid,"Second")); /* Current connection lasts until reboot. */
    reboot();assert(!jr_wifi_is_configured());
    assert(jr_wifi_profiles_list(reply,sizeof(reply)));assert(!strstr(reply,"\"id\":1"));
    /* Slot 0 can be a virtual profile sourced from the old selected record. */
    memset(blobs,0,sizeof(blobs));
    assert(nvs_set_blob(1,"cfg_v2",&original,sizeof(original))==ESP_OK);reboot();
    assert(jr_wifi_profile_delete(0,reply,sizeof(reply)));assert(strstr(reply,"setup_on_restart=1"));
    check_selected("","");reboot();assert(!jr_wifi_is_configured());
    assert(jr_wifi_profiles_list(reply,sizeof(reply)));assert(strstr(reply,"\"profiles\":[]"));

    /* V1.8.4S: last used network goes down; next saved one is attempted. */
    memset(blobs,0,sizeof(blobs));
    assert(nvs_set_blob(1,"cfg_v2",&original,sizeof(original))==ESP_OK);
    reboot();
    assert(save("Backup","backuppass123"));
    station=(esp_netif_t *)(uintptr_t)1;
    wifi_started=true;
    wifi_attempts=1;
    wifi_tried_profiles=0;
    wifi_retry_pending=false;
    simulated_connects=0;
    wifi_event_handler(NULL,WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,NULL);
    jr_wifi_poll();
    assert(simulated_connects==1); /* Retried original before changing network. */
    assert(!strcmp(active.ssid,"Original"));
    wifi_event_handler(NULL,WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,NULL);
    jr_wifi_poll();
    assert(!strcmp(active.ssid,"Backup"));
    assert(!strcmp(simulated_ssid,"Backup"));
    assert(simulated_connects==2);
    assert(simulated_scan_method==WIFI_ALL_CHANNEL_SCAN);
    assert(simulated_sort_method==WIFI_CONNECT_AP_BY_SIGNAL);
    check_selected("Original","password123"); /* Not persisted before success. */
    ip_event_got_ip_t ip={0};
    wifi_event_handler(NULL,IP_EVENT,IP_EVENT_STA_GOT_IP,&ip);
    jr_wifi_poll();
    check_selected("Backup","backuppass123");
    assert(jr_wifi_is_connected());
    assert(wifi_attempts==1 && wifi_tried_profiles==0);
    assert(simulated_rssi_arms==1);

    /* Signal event triggers a scan, but not an immediate forced disconnect. */
    memset(&associated,0,sizeof(associated));
    strcpy((char *)associated.ssid,"Backup");
    associated.bssid[0]=1;associated.rssi=-82;
    memset(scan_candidates,0,sizeof(scan_candidates));
    strcpy((char *)scan_candidates[0].ssid,"Backup");
    scan_candidates[0].bssid[0]=2;scan_candidates[0].rssi=-55;
    scan_candidate_count=1;
    wifi_event_handler(NULL,WIFI_EVENT,WIFI_EVENT_STA_BSS_RSSI_LOW,NULL);
    jr_wifi_poll();assert(simulated_scans==1 && simulated_disconnects==0);
    wifi_event_handler(NULL,WIFI_EVENT,WIFI_EVENT_SCAN_DONE,NULL);
    jr_wifi_poll();assert(simulated_disconnects==1);
    wifi_event_handler(NULL,WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,NULL);
    jr_wifi_poll();assert(simulated_connects==3);
    wifi_event_handler(NULL,IP_EVENT,IP_EVENT_STA_GOT_IP,&ip);
    jr_wifi_poll();assert(simulated_rssi_arms==2);
    /* Never interrupt audio/video with background scanning. */
    simulated_time_us=1000000000LL;simulated_mode=JR_MODE_LIVE;
    wifi_event_handler(NULL,WIFI_EVENT,WIFI_EVENT_STA_BSS_RSSI_LOW,NULL);
    jr_wifi_poll();assert(simulated_scans==1);
    simulated_mode=JR_MODE_IDLE;
    jr_wifi_poll();assert(simulated_scans==2);
    wifi_event_handler(NULL,WIFI_EVENT,WIFI_EVENT_SCAN_DONE,NULL);
    jr_wifi_poll(); /* Returns to idle after second scan. */
    wifi_started=false;
    return 0;
}
'''


class ProfileTests(unittest.TestCase):
    def test_catalog_against_fake_nvs(self):
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            (directory / "sdk_mock.h").write_text(MOCK)
            for header in ["esp_event.h", "esp_err.h", "esp_netif.h", "esp_wifi.h", "esp_wifi_default.h",
                           "nvs.h", "nvs_flash.h", "freertos/FreeRTOS.h", "lwip/inet.h", "esp_timer.h"]:
                path = directory / header
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('#include "sdk_mock.h"\n')
            (directory / "main.c").write_text(HARNESS)
            executable = directory / "test"
            subprocess.run([
                "gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-O1",
                "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
                "-fsanitize=address,undefined", "-I", str(directory), "-I", str(ROOT / "firmware/core"),
                str(directory / "main.c"), "-o", str(executable),
            ], check=True)
            result = subprocess.run([str(executable)], capture_output=True, text=True,
                                    env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        payloads = [json.loads(line.removeprefix("JR_WIFI_PROFILES ")) for line in result.stdout.splitlines()
                    if line.startswith("JR_WIFI_PROFILES ")]
        self.assertEqual([len(p["profiles"]) for p in payloads], [1, 2, 4, 4])
        self.assertNotIn("password", result.stdout)


if __name__ == "__main__":
    unittest.main()
