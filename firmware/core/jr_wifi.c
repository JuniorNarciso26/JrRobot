#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "lwip/inet.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "jr_config.h"
#include "jr_wifi.h"

#define CFG_SCHEMA 0x4a520201U
/* One versioned NVS value avoids mixed old/new fields after a failed write. */
typedef struct {
    uint32_t schema;
    uint8_t use_static;
    char ssid[33], pass[65], host[33], ip[16], gateway[16], mask[16], dns1[16], dns2[16];
} wifi_settings_t;
static wifi_settings_t active;
static bool prepared, start_attempted, wifi_started, driver_initialized;
static esp_netif_t *station;
static esp_event_handler_instance_t wifi_handler, ip_handler;
static bool wifi_handler_set, ip_handler_set;
static portMUX_TYPE state_lock=portMUX_INITIALIZER_UNLOCKED;
static jr_wifi_status_t state={.ip="sem wifi"};

static void set_error(esp_err_t err) {
    portENTER_CRITICAL(&state_lock); state.last_error=err; portEXIT_CRITICAL(&state_lock);
}
static bool copy_checked(char *dst,size_t cap,const char *src) {
    if (!src) src="";
    if (strlen(src)>=cap) return false;
    memcpy(dst,src,strlen(src)+1); return true;
}
static void defaults(wifi_settings_t *c) {
    memset(c,0,sizeof(*c)); c->schema=CFG_SCHEMA; c->use_static=JR_WIFI_USE_STATIC_IP?1:0;
    copy_checked(c->ssid,sizeof(c->ssid),JR_WIFI_SSID);
    copy_checked(c->pass,sizeof(c->pass),JR_WIFI_PASSWORD);
    copy_checked(c->host,sizeof(c->host),JR_WIFI_HOSTNAME);
    copy_checked(c->ip,sizeof(c->ip),JR_WIFI_STATIC_IP);
    copy_checked(c->gateway,sizeof(c->gateway),JR_WIFI_GATEWAY);
    copy_checked(c->mask,sizeof(c->mask),JR_WIFI_SUBNET);
    copy_checked(c->dns1,sizeof(c->dns1),JR_WIFI_DNS1);
    copy_checked(c->dns2,sizeof(c->dns2),JR_WIFI_DNS2);
}
static bool printable(const char *s) {
    for (;*s;s++) if ((unsigned char)*s<32 || (unsigned char)*s==127) return false;
    return true;
}
static bool ipv4(const char *s,uint32_t *out) {
    /* Match the decimal spelling used by the network stack; reject octal-like inputs. */
    uint32_t value=0;
    for (int i=0;i<4;i++) {
        if (*s<'0' || *s>'9') return false;
        const char *begin=s; unsigned part=0;
        while (*s>='0' && *s<='9') {
            part=part*10+(unsigned)(*s++-'0');
            if (part>255 || s-begin>3) return false;
        }
        if (s-begin>1 && *begin=='0') return false;
        value=(value<<8)|part;
        if (i<3) { if (*s++!='.') return false; }
        else if (*s) return false;
    }
    *out=value; return true;
}
static bool unicast(uint32_t ip) { return ip && ip!=UINT32_MAX && (ip>>24)!=127 && (ip>>24)>0 && (ip>>24)<224; }
static bool valid_settings(const wifi_settings_t *c,bool allow_empty) {
    if (c->schema!=CFG_SCHEMA || c->use_static>1) return false;
    const struct {const char *p;size_t n;} fields[]={
        {c->ssid,sizeof(c->ssid)},{c->pass,sizeof(c->pass)},{c->host,sizeof(c->host)},
        {c->ip,sizeof(c->ip)},{c->gateway,sizeof(c->gateway)},{c->mask,sizeof(c->mask)},
        {c->dns1,sizeof(c->dns1)},{c->dns2,sizeof(c->dns2)}};
    for (size_t i=0;i<sizeof(fields)/sizeof(fields[0]);i++)
        if (!memchr(fields[i].p,0,fields[i].n) || !printable(fields[i].p)) return false;
    if (!c->ssid[0]) return allow_empty;
    size_t n=strlen(c->pass);
    if (n && n<8) return false;
    if (n==64) for (size_t i=0;i<n;i++) if (!isxdigit((unsigned char)c->pass[i])) return false;
    n=strlen(c->host);
    if (!n || c->host[0]=='-' || c->host[n-1]=='-') return false;
    for (size_t i=0;i<n;i++) {
        unsigned char ch=(unsigned char)c->host[i];
        if (!((ch>='A'&&ch<='Z')||(ch>='a'&&ch<='z')||(ch>='0'&&ch<='9')||ch=='-')) return false;
    }
    if (c->use_static) {
        uint32_t ip,gw,mask,d1,d2;
        if (!ipv4(c->ip,&ip)||!ipv4(c->gateway,&gw)||!ipv4(c->mask,&mask)||!ipv4(c->dns1,&d1)||!ipv4(c->dns2,&d2)) return false;
        uint32_t hosts=~mask;
        if (!mask || hosts<3 || (hosts&(hosts+1)) || !unicast(ip)||!unicast(gw)||!unicast(d1)||!unicast(d2)) return false;
        if ((ip&mask)!=(gw&mask) || ip==gw || !(ip&hosts) || (ip&hosts)==hosts || !(gw&hosts) || (gw&hosts)==hosts) return false;
    }
    return true;
}
static esp_err_t ensure_nvs(void) {
    /* Never erase credentials automatically on an initialization error. */
    return nvs_flash_init();
}
static void legacy_field(nvs_handle_t h,const char *key,char *dst,size_t cap) {
    size_t n=cap; char tmp[65];
    if (nvs_get_str(h,key,tmp,&n)==ESP_OK && n<=cap) memcpy(dst,tmp,n);
}
void jr_wifi_prepare(void) {
    if (prepared) return;
    defaults(&active);
    esp_err_t err=ensure_nvs();
    if (err==ESP_OK) {
        nvs_handle_t h;
        err=nvs_open("jr_wifi",NVS_READONLY,&h);
        if (err==ESP_OK) {
            wifi_settings_t stored={0}; size_t n=sizeof(stored);
            esp_err_t read=nvs_get_blob(h,"cfg_v2",&stored,&n);
            if (read==ESP_OK && n==sizeof(stored) && valid_settings(&stored,true)) active=stored;
            else if (read==ESP_ERR_NVS_NOT_FOUND) {
                legacy_field(h,"ssid",active.ssid,sizeof(active.ssid));
                legacy_field(h,"pass",active.pass,sizeof(active.pass)); /* Empty is meaningful. */
                legacy_field(h,"host",active.host,sizeof(active.host));
                legacy_field(h,"ip",active.ip,sizeof(active.ip));
                legacy_field(h,"gw",active.gateway,sizeof(active.gateway));
                legacy_field(h,"mask",active.mask,sizeof(active.mask));
                legacy_field(h,"dns1",active.dns1,sizeof(active.dns1));
                legacy_field(h,"dns2",active.dns2,sizeof(active.dns2));
                nvs_get_u8(h,"static",&active.use_static);
            } else { active.ssid[0]='\0'; err=ESP_ERR_INVALID_STATE; }
            nvs_close(h);
        } else if (err==ESP_ERR_NVS_NOT_FOUND) err=ESP_OK;
    }
    if (!valid_settings(&active,true)) { active.ssid[0]='\0'; err=ESP_ERR_INVALID_ARG; }
    if (err!=ESP_OK) active.ssid[0]='\0';
    prepared=true;
    portENTER_CRITICAL(&state_lock);
    state.configured=active.ssid[0]!=0; state.last_error=err;
    portEXIT_CRITICAL(&state_lock);
}
void jr_wifi_get_status(jr_wifi_status_t *out) { if (!out) return; portENTER_CRITICAL(&state_lock); *out=state; portEXIT_CRITICAL(&state_lock); }
bool jr_wifi_is_configured(void) { jr_wifi_status_t s; jr_wifi_get_status(&s); return s.configured; }
bool jr_wifi_is_connected(void) { jr_wifi_status_t s; jr_wifi_get_status(&s); return s.connected; }
bool jr_wifi_network_ready(void) { jr_wifi_status_t s; jr_wifi_get_status(&s); return s.network_ready; }
/* Retained legacy API; the synchronized status snapshot is preferred. */
const char *jr_wifi_ip(void) { return state.ip; }
const char *jr_wifi_ssid(void) { return active.ssid; }

static void wifi_event_handler(void *arg,esp_event_base_t base,int32_t id,void *data) {
    (void)arg;
    if (base==WIFI_EVENT && id==WIFI_EVENT_STA_START) {
        esp_err_t err=esp_wifi_connect(); if (err!=ESP_OK) set_error(err);
    } else if (base==WIFI_EVENT && id==WIFI_EVENT_STA_DISCONNECTED) {
        portENTER_CRITICAL(&state_lock); state.connected=false; memcpy(state.ip,"sem wifi",9); portEXIT_CRITICAL(&state_lock);
        esp_err_t err=esp_wifi_connect(); if (err!=ESP_OK) set_error(err);
    } else if (base==IP_EVENT && id==IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *event=data; char ip[16];
        snprintf(ip,sizeof(ip),IPSTR,IP2STR(&event->ip_info.ip));
        portENTER_CRITICAL(&state_lock); memcpy(state.ip,ip,sizeof(ip)); state.connected=true; state.last_error=ESP_OK; portEXIT_CRITICAL(&state_lock);
        printf("JR_WIFI conectado ip=%s\n",ip);
    }
}
void jr_wifi_start(void) {
    if (start_attempted) return;
    start_attempted=true;
    jr_wifi_prepare();
    if (!active.ssid[0]) { puts("JR_WIFI sem_configuracao use_serial e reinicie_apos_salvar"); return; }
    esp_err_t err=esp_netif_init(); if (err!=ESP_OK) goto fail;
    err=esp_event_loop_create_default(); if (err!=ESP_OK && err!=ESP_ERR_INVALID_STATE) goto fail;
    /* The convenience helper asserts internally; use checked creation instead. */
    esp_netif_config_t netif_config=ESP_NETIF_DEFAULT_WIFI_STA();
    station=esp_netif_new(&netif_config);
    if (!station) { err=ESP_ERR_NO_MEM; goto fail; }
    err=esp_netif_attach_wifi_station(station); if (err!=ESP_OK) goto fail;
    err=esp_wifi_set_default_wifi_sta_handlers(); if (err!=ESP_OK) goto fail;
    err=esp_netif_set_hostname(station,active.host); if (err!=ESP_OK) goto fail;
    if (active.use_static) {
        esp_netif_ip_info_t info={0};
        info.ip.addr=ipaddr_addr(active.ip); info.gw.addr=ipaddr_addr(active.gateway); info.netmask.addr=ipaddr_addr(active.mask);
        err=esp_netif_dhcpc_stop(station); if (err!=ESP_OK && err!=ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) goto fail;
        err=esp_netif_set_ip_info(station,&info); if (err!=ESP_OK) goto fail;
        esp_netif_dns_info_t dns={0}; dns.ip.type=ESP_IPADDR_TYPE_V4;
        dns.ip.u_addr.ip4.addr=ipaddr_addr(active.dns1); err=esp_netif_set_dns_info(station,ESP_NETIF_DNS_MAIN,&dns); if (err!=ESP_OK) goto fail;
        dns.ip.u_addr.ip4.addr=ipaddr_addr(active.dns2); err=esp_netif_set_dns_info(station,ESP_NETIF_DNS_BACKUP,&dns); if (err!=ESP_OK) goto fail;
    }
    wifi_init_config_t init=WIFI_INIT_CONFIG_DEFAULT();
    err=esp_wifi_init(&init); if (err!=ESP_OK) goto fail; driver_initialized=true;
    /* Our versioned record is authoritative; do not keep a second driver copy. */
    err=esp_wifi_set_storage(WIFI_STORAGE_RAM); if (err!=ESP_OK) goto fail;
    err=esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,wifi_event_handler,NULL,&wifi_handler); if (err!=ESP_OK) goto fail; wifi_handler_set=true;
    err=esp_event_handler_instance_register(IP_EVENT,IP_EVENT_STA_GOT_IP,wifi_event_handler,NULL,&ip_handler); if (err!=ESP_OK) goto fail; ip_handler_set=true;
    wifi_config_t config={0};
    memcpy(config.sta.ssid,active.ssid,strlen(active.ssid));
    memcpy(config.sta.password,active.pass,strlen(active.pass));
    config.sta.threshold.authmode=active.pass[0]?WIFI_AUTH_WPA2_PSK:WIFI_AUTH_OPEN;
    err=esp_wifi_set_mode(WIFI_MODE_STA); if (err!=ESP_OK) goto fail;
    err=esp_wifi_set_config(WIFI_IF_STA,&config); if (err!=ESP_OK) goto fail;
    err=esp_wifi_start(); if (err!=ESP_OK) goto fail;
    wifi_started=true;
    portENTER_CRITICAL(&state_lock); state.network_ready=true; portEXIT_CRITICAL(&state_lock);
    puts("JR_WIFI conectando"); return;
fail:
    if (wifi_handler_set) { esp_event_handler_instance_unregister(WIFI_EVENT,ESP_EVENT_ANY_ID,wifi_handler); wifi_handler_set=false; }
    if (ip_handler_set) { esp_event_handler_instance_unregister(IP_EVENT,IP_EVENT_STA_GOT_IP,ip_handler); ip_handler_set=false; }
    if (driver_initialized) { if (wifi_started) esp_wifi_stop(); esp_wifi_deinit(); driver_initialized=false; wifi_started=false; }
    if (station) { esp_netif_destroy_default_wifi(station); station=NULL; }
    portENTER_CRITICAL(&state_lock); state.network_ready=false; state.connected=false; state.last_error=err; portEXIT_CRITICAL(&state_lock);
    printf("JR_WIFI_ERROR start=%s diagnostico_serial_disponivel\n",esp_err_to_name(err));
}
static bool save_record(const wifi_settings_t *c,char *response,unsigned cap) {
    esp_err_t err=ensure_nvs(); nvs_handle_t h;
    if (err==ESP_OK) {
        err=nvs_open("jr_wifi",NVS_READWRITE,&h);
        if (err==ESP_OK) {
            err=nvs_set_blob(h,"cfg_v2",c,sizeof(*c));
            if (err==ESP_OK) err=nvs_commit(h);
            nvs_close(h);
        }
    }
    if (err!=ESP_OK) {
        set_error(err);
        snprintf(response,cap,"JR_WIFI_ERROR persistencia=%s resultado_persistencia_incerto_releia_apos_reiniciar",esp_err_to_name(err)); return false;
    }
    portENTER_CRITICAL(&state_lock); state.configured=c->ssid[0]!=0; state.pending_restart=true; portEXIT_CRITICAL(&state_lock);
    snprintf(response,cap,c->ssid[0]?"JR_WIFI_SALVO pending_restart=1 reinicie_a_placa":"JR_WIFI_LIMPO pending_restart=1 reinicie_a_placa");
    return true;
}
bool jr_wifi_configure(const char *ssid,const char *password,const char *hostname,bool use_static_ip,
    const char *ip,const char *gw,const char *mask,const char *dns1,const char *dns2,char *response,unsigned cap) {
    wifi_settings_t candidate; defaults(&candidate); candidate.use_static=use_static_ip?1:0;
    bool valid=copy_checked(candidate.ssid,sizeof(candidate.ssid),ssid) &&
        copy_checked(candidate.pass,sizeof(candidate.pass),password) &&
        copy_checked(candidate.host,sizeof(candidate.host),hostname&&*hostname?hostname:"jrbot") &&
        copy_checked(candidate.ip,sizeof(candidate.ip),ip&&*ip?ip:JR_WIFI_STATIC_IP) &&
        copy_checked(candidate.gateway,sizeof(candidate.gateway),gw&&*gw?gw:JR_WIFI_GATEWAY) &&
        copy_checked(candidate.mask,sizeof(candidate.mask),mask&&*mask?mask:JR_WIFI_SUBNET) &&
        copy_checked(candidate.dns1,sizeof(candidate.dns1),dns1&&*dns1?dns1:JR_WIFI_DNS1) &&
        copy_checked(candidate.dns2,sizeof(candidate.dns2),dns2&&*dns2?dns2:JR_WIFI_DNS2) && valid_settings(&candidate,false);
    if (!valid) { memset(candidate.pass,0,sizeof(candidate.pass)); snprintf(response,cap,"JR_WIFI_ERROR valores_invalidos_nao_salvos"); return false; }
    bool ok=save_record(&candidate,response,cap); memset(candidate.pass,0,sizeof(candidate.pass)); return ok;
}
bool jr_wifi_clear(char *response,unsigned cap) {
    /* A persistent tombstone also overrides compile-time credentials on reboot. */
    wifi_settings_t empty; defaults(&empty); empty.ssid[0]=0; memset(empty.pass,0,sizeof(empty.pass)); empty.use_static=0;
    return save_record(&empty,response,cap);
}
