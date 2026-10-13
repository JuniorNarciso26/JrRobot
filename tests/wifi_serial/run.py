"""Compile/run the Serial Wi-Fi formatter against host mocks, not ESP-IDF."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2]
sdk=r'''
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_ERR_INVALID_STATE 1
#define ESP_ERR_INVALID_ARG 2
#define ESP_ERR_NO_MEM 3
#define ESP_ERR_TIMEOUT 4
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)(x))
#define portEXIT_CRITICAL(x) ((void)(x))
struct esp_wifi_service { int dummy; };
typedef struct esp_wifi_service esp_wifi_service_t;
typedef void *esp_wifi_service_profile_mgr_t;
typedef void *esp_wifi_service_scan_handle_t;
typedef struct {uint8_t ssid[33];int rssi,authmode;} wifi_ap_record_t;
typedef struct {uint8_t flags,priority;char ssid[33],password[65];} esp_wifi_service_profile_t;
#define ESP_WIFI_SERVICE_PROFILE_FLAG_ENABLED 1
const char *esp_err_to_name(esp_err_t);
esp_err_t esp_wifi_service_get_scan_handle(esp_wifi_service_t *,esp_wifi_service_scan_handle_t *);
esp_err_t esp_wifi_service_scan_request(void *,const void *,void(*)(esp_err_t,uint16_t,const wifi_ap_record_t*,void*),void *);
esp_err_t esp_wifi_service_get_profile_manager(esp_wifi_service_t *,void **);
esp_err_t esp_wifi_service_profile_mgr_foreach(void *,bool(*)(const esp_wifi_service_profile_t*,void*),void*);
esp_err_t esp_wifi_service_profile_mgr_delete(void *,const char *);
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *);
'''
test=r'''
#include <assert.h>
#include "jr_wifi_serial.c"
static struct esp_wifi_service service;
static void (*callback)(esp_err_t,uint16_t,const wifi_ap_record_t*,void*);
static esp_err_t join_result;
const char *esp_err_to_name(esp_err_t e){return e==ESP_ERR_TIMEOUT?"ESP_ERR_TIMEOUT":"mock";}
struct esp_wifi_service *jr_wifi_service_handle(void){return &service;}
const char *jr_wifi_ip(void){return "192.168.1.4";}
esp_err_t jr_wifi_serial_apply_credentials(const char *s,const char *p){assert(!strcmp(s,"Casa"));assert(!strcmp(p,"password123"));return join_result;}
esp_err_t esp_wifi_service_get_scan_handle(esp_wifi_service_t *s,void **a){(void)s;*a=&service;return ESP_OK;}
esp_err_t esp_wifi_service_scan_request(void *a,const void *c,void(*cb)(esp_err_t,uint16_t,const wifi_ap_record_t*,void*),void *ctx){(void)a;(void)c;(void)ctx;callback=cb;return ESP_OK;}
esp_err_t esp_wifi_service_get_profile_manager(esp_wifi_service_t *s,void **m){(void)s;*m=&service;return ESP_OK;}
esp_err_t esp_wifi_service_profile_mgr_foreach(void *m,bool(*cb)(const esp_wifi_service_profile_t*,void*),void *ctx){(void)m;esp_wifi_service_profile_t p={.ssid="Cafe | rede",.password="SECRET",.flags=1,.priority=10};cb(&p,ctx);return ESP_OK;}
esp_err_t esp_wifi_service_profile_mgr_delete(void *m,const char *s){(void)m;assert(!strcmp(s,"Cafe | rede"));return ESP_OK;}
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *ap){memcpy(ap->ssid,"Cafe | rede",12);return ESP_OK;}
int main(void){char out[2048];assert(jr_wifi_serial_scan(true,out,sizeof(out)));assert(strstr(out,"pending=1"));wifi_ap_record_t ap={.ssid="Cafe | rede",.rssi=-42,.authmode=3};callback(ESP_OK,1,&ap,NULL);assert(jr_wifi_serial_scan(false,out,sizeof(out)));assert(strstr(out,"Cafe%20%7C%20rede,-42,3"));assert(!jr_wifi_serial_scan(false,out,8));assert(jr_wifi_serial_profiles(out,sizeof(out)));assert(strstr(out,"Cafe%20%7C%20rede,1,10"));assert(!strstr(out,"SECRET"));assert(jr_wifi_serial_delete("Cafe | rede",out,sizeof(out)));assert(jr_wifi_serial_join("Casa","password123",out,sizeof(out)));join_result=ESP_ERR_TIMEOUT;assert(!jr_wifi_serial_join("Casa","password123",out,sizeof(out)));assert(strstr(out,"ESP_ERR_TIMEOUT"));assert(!jr_wifi_serial_join("Casa","short",out,sizeof(out)));puts("PASS Serial Wi-Fi scan, escaped SSIDs, bounded replies, no passwords, deletion, connection errors");}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'freertos').mkdir();(p/'sdk.h').write_text(sdk)
 for name in ['esp_err.h','esp_wifi.h','esp_wifi_service.h','freertos/FreeRTOS.h']:(p/name).write_text('#include "sdk.h"\n')
 (p/'test.c').write_text(test)
 subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=undefined','-I',str(p),'-I',str(root/'firmware/core'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
