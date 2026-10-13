"""Host validation of mDNS startup and memory diagnostics; no ESP-IDF build."""
from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parents[2]
sdk=r'''
#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_ERR_INVALID_STATE 1
#define MALLOC_CAP_INTERNAL 1
#define MALLOC_CAP_8BIT 2
#define MALLOC_CAP_DMA 4
#define MALLOC_CAP_SPIRAM 8
#define CONFIG_MDNS_MEMORY_ALLOC_SPIRAM 1
#define CONFIG_MDNS_TASK_STACK_SIZE 4096
#define CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE 4096
#define CONFIG_LWIP_MAX_SOCKETS 32
#define ESP_LOGI(t,...) do{(void)(t);printf(__VA_ARGS__);puts("");}while(0)
#define ESP_RETURN_ON_FALSE(c,e,t,m) do{(void)(t);if(!(c))return (e);}while(0)
#define ESP_RETURN_ON_ERROR(expr,t,m) do{(void)(t);int r=(expr);if(r)return r;}while(0)
typedef struct {int n;} esp_netif_t;
typedef struct {struct {uint32_t addr;} ip;} esp_netif_ip_info_t;
enum {MDNS_EVENT_ENABLE_IP4=1,MDNS_EVENT_ANNOUNCE_IP4=2};
esp_netif_t *esp_netif_get_handle_from_ifkey(const char *);
esp_err_t esp_netif_set_hostname(esp_netif_t *,const char *);
esp_err_t esp_netif_get_ip_info(esp_netif_t *,esp_netif_ip_info_t *);
esp_err_t mdns_init(void);
esp_err_t mdns_hostname_set(const char *);
esp_err_t mdns_instance_name_set(const char *);
esp_err_t mdns_service_add(const char *,const char *,const char *,uint16_t,void *,size_t);
esp_err_t mdns_netif_action(esp_netif_t *,int);
void mdns_free(void);
size_t heap_caps_get_free_size(unsigned);
size_t heap_caps_get_largest_free_block(unsigned);
'''
test=r'''
#include <assert.h>
#include <string.h>
#include "jr_mdns.c"
static esp_netif_t sta;
static unsigned init_calls,enabled,announced,dma_queries;
esp_netif_t *esp_netif_get_handle_from_ifkey(const char *key){assert(!strcmp(key,"WIFI_STA_DEF"));return &sta;}
esp_err_t esp_netif_set_hostname(esp_netif_t *s,const char *h){assert(s==&sta&&!strcmp(h,"jrbot"));return 0;}
esp_err_t esp_netif_get_ip_info(esp_netif_t *s,esp_netif_ip_info_t *ip){(void)s;ip->ip.addr=1;return 0;}
esp_err_t mdns_init(void){init_calls++;return 0;}
esp_err_t mdns_hostname_set(const char *h){assert(!strcmp(h,"jrbot"));return 0;}
esp_err_t mdns_instance_name_set(const char *h){assert(!strcmp(h,"JrBot"));return 0;}
esp_err_t mdns_service_add(const char *i,const char *s,const char *p,uint16_t port,void *txt,size_t n){(void)i;(void)txt;assert(!strcmp(s,"_http")&&!strcmp(p,"_tcp")&&port==80&&n==0);return 0;}
esp_err_t mdns_netif_action(esp_netif_t *s,int a){assert(s==&sta);if(a==MDNS_EVENT_ENABLE_IP4)enabled++;else announced++;return 0;}
void mdns_free(void){}
size_t heap_caps_get_free_size(unsigned c){if(c&MALLOC_CAP_DMA)dma_queries++;return 100000;}
size_t heap_caps_get_largest_free_block(unsigned c){if(c&MALLOC_CAP_DMA)dma_queries++;return 32768;}
int main(void){assert(jr_mdns_start()==ESP_OK);assert(jr_mdns_start()==ESP_OK);assert(init_calls==1&&enabled==2&&announced==2&&dma_queries==4);puts("PASS mDNS memory reporting and DHCP announcements (mock SDK)");}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'sdk.h').write_text(sdk)
 for name in ['esp_err.h','esp_check.h','esp_log.h','esp_netif.h','mdns.h','esp_heap_caps.h','sdkconfig.h']:(p/name).write_text('#include "sdk.h"\n')
 (p/'test.c').write_text(test)
 subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=undefined','-I',str(p),'-I',str(root/'firmware/core'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
