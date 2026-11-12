#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_timer.h"
#include "jr_mode_manager.h"
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
/* V1.8.4S: keep network failover state in the existing Wi-Fi module; no extra task. */
#define PROFILE_LIMIT 4
#define WIFI_ATTEMPTS_PER_PROFILE 2
/* Event-triggered AP selection: no periodic scan, no extra task. */
#define JR_ROAM_RSSI_LOW_DBM (-70)
#define JR_ROAM_MIN_GAIN_DB 12
#define JR_ROAM_COOLDOWN_MS 90000U
#define JR_ROAM_SCAN_CAP 8
static bool roam_low_pending, roam_scan_inflight, roam_scan_done;
static bool roam_switch_requested, roam_reconnect_pending, roam_rearm_pending;
static bool roam_has_scanned;
static uint32_t roam_last_scan_ms;
static bool wifi_retry_pending, wifi_got_ip_pending, wifi_fallback_active;
static unsigned wifi_attempts;
static uint8_t wifi_tried_profiles;
static esp_err_t profile_get(nvs_handle_t h,unsigned id,wifi_settings_t *out);

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

/* Apply IP mode before connecting another saved profile. */
static esp_err_t wifi_addressing(const wifi_settings_t *config) {
    if (config->use_static) {
        esp_err_t err=esp_netif_dhcpc_stop(station);
        if (err!=ESP_OK && err!=ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) return err;
        esp_netif_ip_info_t info={0};
        info.ip.addr=ipaddr_addr(config->ip);
        info.gw.addr=ipaddr_addr(config->gateway);
        info.netmask.addr=ipaddr_addr(config->mask);
        err=esp_netif_set_ip_info(station,&info); if (err!=ESP_OK) return err;
        esp_netif_dns_info_t dns={0}; dns.ip.type=ESP_IPADDR_TYPE_V4;
        dns.ip.u_addr.ip4.addr=ipaddr_addr(config->dns1);
        err=esp_netif_set_dns_info(station,ESP_NETIF_DNS_MAIN,&dns); if (err!=ESP_OK) return err;
        dns.ip.u_addr.ip4.addr=ipaddr_addr(config->dns2);
        return esp_netif_set_dns_info(station,ESP_NETIF_DNS_BACKUP,&dns);
    }
    esp_err_t err=esp_netif_dhcpc_start(station);
    return err==ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED?ESP_OK:err;
}
static esp_err_t wifi_apply_profile(const wifi_settings_t *config) {
    esp_err_t err=esp_netif_set_hostname(station,config->host); if (err!=ESP_OK) return err;
    err=wifi_addressing(config); if (err!=ESP_OK) return err;
    wifi_config_t sta={0};
    memcpy(sta.sta.ssid,config->ssid,strlen(config->ssid));
    memcpy(sta.sta.password,config->pass,strlen(config->pass));
    sta.sta.threshold.authmode=config->pass[0]?WIFI_AUTH_WPA2_PSK:WIFI_AUTH_OPEN;
    /* ESP-IDF native scan: choose strongest compatible AP for the given SSID. */
    sta.sta.scan_method=WIFI_ALL_CHANNEL_SCAN;
    sta.sta.sort_method=WIFI_CONNECT_AP_BY_SIGNAL;
    err=esp_wifi_set_config(WIFI_IF_STA,&sta);
    memset(&sta,0,sizeof(sta));
    return err;
}
/* Read candidates only on the main loop, never in an ESP-IDF event callback. */
static bool wifi_choose_next(void) {
    nvs_handle_t h;
    esp_err_t err=nvs_open("jr_wifi",NVS_READONLY,&h);
    if (err!=ESP_OK) return false;
    wifi_settings_t candidate={0};
    bool chosen=false;
    for (unsigned id=0;id<PROFILE_LIMIT;id++) {
        uint8_t bit=(uint8_t)(1u<<id);
        if (wifi_tried_profiles&bit) continue;
        wifi_tried_profiles|=bit;
        err=profile_get(h,id,&candidate);
        if (err==ESP_ERR_NVS_NOT_FOUND) continue;
        if (err!=ESP_OK) {
            printf("JR_WIFI_FAILOVER invalid_profile=%u error=%s\n",id,esp_err_to_name(err));
            continue;
        }
        if (!candidate.ssid[0] || !strcmp(candidate.ssid,active.ssid)) continue;
        err=wifi_apply_profile(&candidate);
        if (err!=ESP_OK) {
            printf("JR_WIFI_FAILOVER profile=%u config_error=%s\n",id,esp_err_to_name(err));
            continue;
        }
        active=candidate;
        wifi_fallback_active=true;
        wifi_attempts=1;
        chosen=true;
        printf("JR_WIFI_FAILOVER attempting_profile=%u ssid=%s\n",id,active.ssid);
        err=esp_wifi_connect();
        if (err!=ESP_OK) {
            set_error(err);
            wifi_retry_pending=true;
        }
        break;
    }
    memset(candidate.pass,0,sizeof(candidate.pass));
    nvs_close(h);
    return chosen;
}
/* Persist the *successful* fallback as the next boot preference. No write on failure. */
static void wifi_remember_connected(void) {
    if (!wifi_fallback_active) return;
    wifi_fallback_active=false;
    nvs_handle_t h;
    esp_err_t err=nvs_open("jr_wifi",NVS_READWRITE,&h);
    if (err!=ESP_OK) { set_error(err); return; }
    err=nvs_set_blob(h,"cfg_v2",&active,sizeof(active));
    if (err==ESP_OK) err=nvs_commit(h);
    nvs_close(h);
    if (err!=ESP_OK) {
        set_error(err);
        printf("JR_WIFI_FAILOVER connected_not_persisted=%s\n",esp_err_to_name(err));
    } else {
        printf("JR_WIFI_FAILOVER preferred_network=%s saved=1\n",active.ssid);
    }
}

/* ESP-IDF raises RSSI_LOW, but does not roam automatically on this setting.
 * Scan only after the low signal event, and only while the robot is idle.
 * The Wi-Fi driver owns the scan buffers; collect and release records once.
 */
static void wifi_roam_evaluate_scan(void) {
    wifi_ap_record_t found[JR_ROAM_SCAN_CAP]={0};
    uint16_t count=JR_ROAM_SCAN_CAP;
    esp_err_t err=esp_wifi_scan_get_ap_records(&count,found);
    if(err!=ESP_OK) {
        printf("JR_WIFI_ROAM scan_results_error=%s\n",esp_err_to_name(err));
        roam_rearm_pending=true;
        return;
    }
    wifi_ap_record_t current={0};
    if(!jr_wifi_is_connected() || jr_mode_get()!=JR_MODE_IDLE ||
       esp_wifi_sta_get_ap_info(&current)!=ESP_OK) {
        puts("JR_WIFI_ROAM decision=skip_connection_or_mode_changed");
        roam_rearm_pending=true;
        return;
    }
    int best=(int)current.rssi;
    bool better=false;
    for(uint16_t i=0;i<count;i++) {
        if(strcmp((const char *)found[i].ssid,active.ssid) ||
           !memcmp(found[i].bssid,current.bssid,sizeof(current.bssid))) continue;
        /* All candidate APs have the same SSID; the driver re-checks auth. */
        if((int)found[i].rssi>=((int)current.rssi+JR_ROAM_MIN_GAIN_DB) &&
           (int)found[i].rssi>best) {
            best=found[i].rssi;
            better=true;
        }
    }
    printf("JR_WIFI_ROAM scan_done candidates=%u current_rssi=%d best_rssi=%d min_gain=%d decision=%s\n",
           (unsigned)count,(int)current.rssi,best,JR_ROAM_MIN_GAIN_DB,
           better?"switch":"keep");
    if(!better) { roam_rearm_pending=true; return; }
    /* Reconnect with all-channel selection; never fix the driver to a BSSID. */
    portENTER_CRITICAL(&state_lock);
    roam_switch_requested=true;
    portEXIT_CRITICAL(&state_lock);
    err=esp_wifi_disconnect();
    if(err!=ESP_OK) {
        portENTER_CRITICAL(&state_lock);
        roam_switch_requested=false;
        portEXIT_CRITICAL(&state_lock);
        roam_rearm_pending=true;
        printf("JR_WIFI_ROAM disconnect_error=%s\n",esp_err_to_name(err));
    }
}
static void wifi_roam_poll(void) {
    uint32_t now=(uint32_t)(esp_timer_get_time()/1000ULL);
    bool rejoin=false, done=false, weak=false, connected=false;
    portENTER_CRITICAL(&state_lock);
    rejoin=roam_reconnect_pending;roam_reconnect_pending=false;
    done=roam_scan_done;roam_scan_done=false;
    weak=roam_low_pending;
    connected=state.connected;
    portEXIT_CRITICAL(&state_lock);
    if(rejoin) {
        esp_err_t err=esp_wifi_connect();
        printf("JR_WIFI_ROAM reconnect_same_ssid=%s result=%s\n",active.ssid,esp_err_to_name(err));
        if(err!=ESP_OK) {
            set_error(err);
            portENTER_CRITICAL(&state_lock);
            wifi_retry_pending=true;
            portEXIT_CRITICAL(&state_lock);
        }
        return;
    }
    if(done) { wifi_roam_evaluate_scan();return; }
    if(!connected) return;
    if(weak && jr_mode_get()==JR_MODE_IDLE &&
       (!roam_has_scanned || (uint32_t)(now-roam_last_scan_ms)>=JR_ROAM_COOLDOWN_MS)) {
        wifi_scan_config_t options={0};
        options.ssid=(uint8_t *)active.ssid;
        portENTER_CRITICAL(&state_lock);
        roam_scan_inflight=true;
        portEXIT_CRITICAL(&state_lock);
        esp_err_t err=esp_wifi_scan_start(&options,false);
        if(err==ESP_OK) {
            portENTER_CRITICAL(&state_lock);
            roam_low_pending=false;
            portEXIT_CRITICAL(&state_lock);
            roam_last_scan_ms=now;
            roam_has_scanned=true;
            printf("JR_WIFI_ROAM event=scan_started threshold_dbm=%d ssid=%s\n",
                   JR_ROAM_RSSI_LOW_DBM,active.ssid);
        } else {
            roam_last_scan_ms=now;
            roam_has_scanned=true;
            roam_rearm_pending=true;
            portENTER_CRITICAL(&state_lock);
            roam_low_pending=false;
            roam_scan_inflight=false;
            portEXIT_CRITICAL(&state_lock);
            printf("JR_WIFI_ROAM event=scan_deferred error=%s\n",esp_err_to_name(err));
        }
        return;
    }
    if(roam_rearm_pending &&
       (!roam_has_scanned || (uint32_t)(now-roam_last_scan_ms)>=JR_ROAM_COOLDOWN_MS)) {
        roam_rearm_pending=false;
        esp_err_t err=esp_wifi_set_rssi_threshold(JR_ROAM_RSSI_LOW_DBM);
        if(err!=ESP_OK)printf("JR_WIFI_ROAM rearm_error=%s\n",esp_err_to_name(err));
    }
}

/* Called by app_main; do not perform storage/driver configuration in event handler. */
void jr_wifi_poll(void) {
    if (!wifi_started) return;
    wifi_roam_poll();
    bool connected, retry, ip_ready;
    portENTER_CRITICAL(&state_lock);
    connected=state.connected;
    retry=wifi_retry_pending; wifi_retry_pending=false;
    ip_ready=wifi_got_ip_pending; wifi_got_ip_pending=false;
    portEXIT_CRITICAL(&state_lock);
    if (ip_ready && connected) {
        wifi_attempts=1;
        wifi_tried_profiles=0;
        wifi_remember_connected();
        roam_low_pending=false;
        roam_rearm_pending=false;
        esp_err_t arm=esp_wifi_set_rssi_threshold(JR_ROAM_RSSI_LOW_DBM);
        printf("JR_WIFI_ROAM monitor_threshold=%d result=%s\n",
               JR_ROAM_RSSI_LOW_DBM,esp_err_to_name(arm));
        return;
    }
    if (!retry || connected) return;
    if (wifi_attempts<WIFI_ATTEMPTS_PER_PROFILE) {
        wifi_attempts++;
        printf("JR_WIFI_RETRY active_profile=\"%s\" attempt=%u max=%u\n",
               active.ssid,wifi_attempts,(unsigned)WIFI_ATTEMPTS_PER_PROFILE);
        esp_err_t err=esp_wifi_connect();
        if (err!=ESP_OK) {
            set_error(err);
            portENTER_CRITICAL(&state_lock); wifi_retry_pending=true; portEXIT_CRITICAL(&state_lock);
        }
        return;
    }
    if (!wifi_choose_next())
        printf("JR_WIFI_FAILOVER exhausted active_profile=\"%s\" attempts=%u setup_recovery_available=1\n",
               active.ssid,wifi_attempts);
}
static void wifi_event_handler(void *arg,esp_event_base_t base,int32_t id,void *data) {
    (void)arg;
    if (base==WIFI_EVENT && id==WIFI_EVENT_STA_START) {
        esp_err_t err=esp_wifi_connect(); if (err!=ESP_OK) set_error(err);
    } else if (base==WIFI_EVENT && id==WIFI_EVENT_STA_DISCONNECTED) {
        portENTER_CRITICAL(&state_lock);
        bool voluntary=roam_switch_requested;
        roam_switch_requested=false;
        state.connected=false; memcpy(state.ip,"sem wifi",9);
        roam_low_pending=false;
        roam_reconnect_pending=voluntary;
        wifi_retry_pending=!voluntary;
        portEXIT_CRITICAL(&state_lock);
    } else if (base==WIFI_EVENT && id==WIFI_EVENT_STA_BSS_RSSI_LOW) {
        portENTER_CRITICAL(&state_lock);
        roam_low_pending=true;
        portEXIT_CRITICAL(&state_lock);
        printf("JR_WIFI_ROAM event=rssi_low threshold_dbm=%d\n",JR_ROAM_RSSI_LOW_DBM);
    } else if (base==WIFI_EVENT && id==WIFI_EVENT_SCAN_DONE) {
        portENTER_CRITICAL(&state_lock);
        if(roam_scan_inflight) {
            roam_scan_inflight=false;
            roam_scan_done=true;
        }
        portEXIT_CRITICAL(&state_lock);
    } else if (base==IP_EVENT && id==IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *event=data; char ip[16];
        snprintf(ip,sizeof(ip),IPSTR,IP2STR(&event->ip_info.ip));
        portENTER_CRITICAL(&state_lock);
        memcpy(state.ip,ip,sizeof(ip)); state.connected=true; state.last_error=ESP_OK;
        wifi_retry_pending=false; wifi_got_ip_pending=true;
        portEXIT_CRITICAL(&state_lock);
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
    /* Addressing is configured after the STA driver is initialized. */
    wifi_init_config_t init=WIFI_INIT_CONFIG_DEFAULT();
    err=esp_wifi_init(&init); if (err!=ESP_OK) goto fail; driver_initialized=true;
    /* Our versioned record is authoritative; do not keep a second driver copy. */
    err=esp_wifi_set_storage(WIFI_STORAGE_RAM); if (err!=ESP_OK) goto fail;
    err=esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,wifi_event_handler,NULL,&wifi_handler); if (err!=ESP_OK) goto fail; wifi_handler_set=true;
    err=esp_event_handler_instance_register(IP_EVENT,IP_EVENT_STA_GOT_IP,wifi_event_handler,NULL,&ip_handler); if (err!=ESP_OK) goto fail; ip_handler_set=true;
    err=esp_wifi_set_mode(WIFI_MODE_STA); if (err!=ESP_OK) goto fail;
    err=wifi_apply_profile(&active); if (err!=ESP_OK) goto fail;
    wifi_attempts=1; wifi_tried_profiles=0;
    wifi_retry_pending=false; wifi_got_ip_pending=false; wifi_fallback_active=false;
    roam_low_pending=false; roam_scan_inflight=false; roam_scan_done=false;
    roam_switch_requested=false; roam_reconnect_pending=false; roam_rearm_pending=false;
    roam_has_scanned=false; roam_last_scan_ms=0;
    err=esp_wifi_start(); if (err!=ESP_OK) goto fail;
    wifi_started=true;
    portENTER_CRITICAL(&state_lock); state.network_ready=true; portEXIT_CRITICAL(&state_lock);
    printf("JR_WIFI connecting active_profile=\"%s\"\n",active.ssid); return;
fail:
    jr_wifi_stop();
    set_error(err);
    printf("JR_WIFI_ERROR start=%s diagnostico_serial_disponivel\n",esp_err_to_name(err));
}
void jr_wifi_stop(void) {
    wifi_retry_pending=false; wifi_got_ip_pending=false; wifi_fallback_active=false;
    roam_low_pending=false; roam_scan_inflight=false; roam_scan_done=false;
    roam_switch_requested=false; roam_reconnect_pending=false; roam_rearm_pending=false;
    roam_has_scanned=false; roam_last_scan_ms=0;
    wifi_attempts=0; wifi_tried_profiles=0;
    if (wifi_handler_set) { esp_event_handler_instance_unregister(WIFI_EVENT,ESP_EVENT_ANY_ID,wifi_handler); wifi_handler_set=false; }
    if (ip_handler_set) { esp_event_handler_instance_unregister(IP_EVENT,IP_EVENT_STA_GOT_IP,ip_handler); ip_handler_set=false; }
    if (driver_initialized) { if (wifi_started) esp_wifi_stop(); esp_wifi_deinit(); driver_initialized=false; wifi_started=false; }
    if (station) { esp_netif_destroy_default_wifi(station); station=NULL; }
    portENTER_CRITICAL(&state_lock);
    state.network_ready=false;
    state.connected=false;
    memcpy(state.ip,"sem wifi",9);
    portEXIT_CRITICAL(&state_lock);
}
static bool settings_equal(const wifi_settings_t *a,const wifi_settings_t *b) {
    return a->schema==b->schema && a->use_static==b->use_static &&
        !strcmp(a->ssid,b->ssid) && !strcmp(a->pass,b->pass) && !strcmp(a->host,b->host) &&
        !strcmp(a->ip,b->ip) && !strcmp(a->gateway,b->gateway) && !strcmp(a->mask,b->mask) &&
        !strcmp(a->dns1,b->dns1) && !strcmp(a->dns2,b->dns2);
}
static bool save_record(const wifi_settings_t *c,char *response,unsigned cap) {
    esp_err_t err=ensure_nvs(); nvs_handle_t h;
    if (err==ESP_OK) {
        err=nvs_open("jr_wifi",NVS_READWRITE,&h);
        if (err==ESP_OK) {
            wifi_settings_t saved; size_t size=sizeof(saved);
            esp_err_t read=nvs_get_blob(h,"cfg_v2",&saved,&size);
            bool unchanged=read==ESP_OK && size==sizeof(saved) && valid_settings(&saved,true) &&
                settings_equal(c,&saved) && settings_equal(c,&active);
            memset(saved.pass,0,sizeof(saved.pass));
            if (unchanged) {
                nvs_close(h);
                portENTER_CRITICAL(&state_lock); state.pending_restart=false; portEXIT_CRITICAL(&state_lock);
                snprintf(response,cap,c->ssid[0]?"JR_WIFI_SALVO pending_restart=0 configuracao_ja_ativa":"JR_WIFI_LIMPO pending_restart=0 configuracao_ja_ativa");
                return true;
            }
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
static esp_err_t profile_read(nvs_handle_t h,unsigned id,wifi_settings_t *out) {
    char key[16]; snprintf(key,sizeof(key),"p%u",id);
    size_t size=sizeof(*out);
    esp_err_t err=nvs_get_blob(h,key,out,&size);
    if (err==ESP_OK && (size!=sizeof(*out) || !valid_settings(out,false))) return ESP_ERR_INVALID_ARG;
    return err;
}
static esp_err_t selected_read(nvs_handle_t h,wifi_settings_t *out) {
    size_t size=sizeof(*out);
    esp_err_t err=nvs_get_blob(h,"cfg_v2",out,&size);
    if (err==ESP_ERR_NVS_NOT_FOUND) { *out=active; return ESP_OK; }
    if (err==ESP_OK && (size!=sizeof(*out) || !valid_settings(out,true))) return ESP_ERR_INVALID_ARG;
    return err;
}
static esp_err_t profile_get(nvs_handle_t h,unsigned id,wifi_settings_t *out) {
    esp_err_t err=profile_read(h,id,out);
    if (err!=ESP_ERR_NVS_NOT_FOUND || id!=0) return err;
    /* Before the first save, expose the existing approved configuration as slot 0. */
    for (unsigned i=1;i<PROFILE_LIMIT;i++) {
        err=profile_read(h,i,out);
        if (err==ESP_OK) return ESP_ERR_NVS_NOT_FOUND;
        if (err!=ESP_ERR_NVS_NOT_FOUND) return err;
    }
    err=selected_read(h,out);
    return err==ESP_OK && !out->ssid[0]?ESP_ERR_NVS_NOT_FOUND:err;
}
static bool profile_error(esp_err_t err,char *response,unsigned cap) {
    snprintf(response,cap,"JR_WIFI_ERROR profiles=%s",esp_err_to_name(err));
    return false;
}
static bool profile_store(const wifi_settings_t *candidate,char *response,unsigned cap) {
    nvs_handle_t h;
    esp_err_t err=ensure_nvs();
    if (err==ESP_OK) err=nvs_open("jr_wifi",NVS_READWRITE,&h);
    if (err!=ESP_OK) return profile_error(err,response,cap);
    int found=-1,empty=-1; bool any=false;
    wifi_settings_t entry;
    for (unsigned i=0;i<PROFILE_LIMIT;i++) {
        err=profile_read(h,i,&entry);
        if (err==ESP_OK) {
            any=true;
            if (!strcmp(entry.ssid,candidate->ssid)) found=(int)i;
        } else if (err==ESP_ERR_NVS_NOT_FOUND) { if (empty<0) empty=(int)i; }
        else goto done;
    }
    if (!any) {
        err=selected_read(h,&entry);
        if (err!=ESP_OK) goto done;
        if (entry.ssid[0] && strcmp(entry.ssid,candidate->ssid)) {
            err=nvs_set_blob(h,"p0",&entry,sizeof(entry));
            if (err!=ESP_OK) goto done;
            empty=1;
        }
    }
    int slot=found>=0?found:empty;
    if (slot<0) {
        nvs_close(h); memset(entry.pass,0,sizeof(entry.pass));
        snprintf(response,cap,"JR_WIFI_ERROR profiles=full exclua_uma_rede_inativa_limite_4");
        return false;
    }
    char key[16]; snprintf(key,sizeof(key),"p%u",(unsigned)slot);
    err=nvs_set_blob(h,key,candidate,sizeof(*candidate));
    if (err==ESP_OK) err=nvs_commit(h);
done:
    nvs_close(h); memset(entry.pass,0,sizeof(entry.pass));
    if (err!=ESP_OK) return profile_error(err,response,cap);
    snprintf(response,cap,"JR_WIFI_PROFILE_SAVED conexao_atual_preservada=1");
    return true;
}
static void profile_ssid_encode(const char *ssid,char *out) {
    static const char hex[]="0123456789ABCDEF";
    unsigned n=0;
    for (unsigned i=0;i<32 && ssid[i];i++) {
        unsigned byte=(unsigned char)ssid[i];
        out[n++]='%'; out[n++]=hex[byte>>4]; out[n++]=hex[byte&15];
    }
    out[n]=0;
}
bool jr_wifi_profiles_list(char *response,unsigned cap) {
    nvs_handle_t h;
    esp_err_t err=ensure_nvs();
    if (err==ESP_OK) err=nvs_open("jr_wifi",NVS_READONLY,&h);
    if (err==ESP_ERR_NVS_NOT_FOUND) { snprintf(response,cap,"JR_WIFI_PROFILES {\"profiles\":[]}"); return true; }
    if (err!=ESP_OK) return profile_error(err,response,cap);
    wifi_settings_t selected,entry;
    err=selected_read(h,&selected);
    unsigned shown=0; size_t used=0;
    if (err==ESP_OK) {
        int n=snprintf(response,cap,"JR_WIFI_PROFILES {\"profiles\":[");
        if (n<0 || (unsigned)n>=cap) err=ESP_ERR_INVALID_SIZE; else used=(size_t)n;
    }
    for (unsigned i=0;err==ESP_OK && i<PROFILE_LIMIT;i++) {
        err=profile_get(h,i,&entry);
        if (err==ESP_ERR_NVS_NOT_FOUND) { err=ESP_OK; continue; }
        if (err!=ESP_OK) break;
        char encoded[97]; profile_ssid_encode(entry.ssid,encoded);
        int n=snprintf(response+used,cap-used,
            "%s{\"id\":%u,\"ssid_pct\":\"%s\",\"mode\":\"%s\",\"selected\":%s,\"protected\":%s}",
            shown?",":"",i,encoded,entry.use_static?"IP_FIXO":"DHCP",
            !strcmp(entry.ssid,selected.ssid)?"true":"false",entry.pass[0]?"true":"false");
        if (n<0 || (size_t)n>=cap-used) { err=ESP_ERR_INVALID_SIZE; break; }
        used+=(size_t)n; shown++;
    }
    if (err==ESP_OK) {
        int n=snprintf(response+used,cap-used,"]}");
        if (n<0 || (size_t)n>=cap-used) err=ESP_ERR_INVALID_SIZE;
    }
    nvs_close(h); memset(selected.pass,0,sizeof(selected.pass)); memset(entry.pass,0,sizeof(entry.pass));
    return err==ESP_OK?true:profile_error(err,response,cap);
}
bool jr_wifi_profile_use(unsigned id,char *response,unsigned cap) {
    if (id>=PROFILE_LIMIT) return profile_error(ESP_ERR_INVALID_ARG,response,cap);
    nvs_handle_t h; wifi_settings_t entry;
    esp_err_t err=ensure_nvs();
    if (err==ESP_OK) err=nvs_open("jr_wifi",NVS_READONLY,&h);
    if (err!=ESP_OK) return profile_error(err,response,cap);
    err=profile_get(h,id,&entry); nvs_close(h);
    bool ok=err==ESP_OK?save_record(&entry,response,cap):profile_error(err,response,cap);
    memset(entry.pass,0,sizeof(entry.pass));
    return ok;
}
bool jr_wifi_profile_delete(unsigned id,char *response,unsigned cap) {
    if (id>=PROFILE_LIMIT) return profile_error(ESP_ERR_INVALID_ARG,response,cap);
    nvs_handle_t h; wifi_settings_t entry={0},selected={0};
    esp_err_t err=ensure_nvs();
    if (err==ESP_OK) err=nvs_open("jr_wifi",NVS_READWRITE,&h);
    if (err!=ESP_OK) return profile_error(err,response,cap);
    err=profile_read(h,id,&entry);
    bool stored=err==ESP_OK;
    if (err==ESP_ERR_NVS_NOT_FOUND) err=profile_get(h,id,&entry);
    if (err==ESP_OK) err=selected_read(h,&selected);
    bool clear_selection=err==ESP_OK && !strcmp(entry.ssid,selected.ssid);
    if (clear_selection) {
        /* Keep a tombstone so compile-time defaults cannot restore this SSID. */
        selected.ssid[0]=0;
        memset(selected.pass,0,sizeof(selected.pass));
        selected.use_static=0;
        err=nvs_set_blob(h,"cfg_v2",&selected,sizeof(selected));
    }
    if (err==ESP_OK && stored) {
        char key[16]; snprintf(key,sizeof(key),"p%u",id);
        err=nvs_erase_key(h,key);
    }
    if (err==ESP_OK) err=nvs_commit(h);
    nvs_close(h); memset(entry.pass,0,sizeof(entry.pass)); memset(selected.pass,0,sizeof(selected.pass));
    if (err!=ESP_OK) return profile_error(err,response,cap);
    bool pending_restart;
    portENTER_CRITICAL(&state_lock);
    if (clear_selection) { state.configured=false; state.pending_restart=true; }
    pending_restart=state.pending_restart;
    portEXIT_CRITICAL(&state_lock);
    snprintf(response,cap,"JR_WIFI_PROFILE_DELETED id=%u pending_restart=%d setup_on_restart=%d conexao_atual_preservada=1",
             id,pending_restart?1:0,clear_selection?1:0);
    return true;
}
static bool configure_record(bool profile_only,const char *ssid,const char *password,const char *hostname,bool use_static_ip,
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
    bool ok=profile_store(&candidate,response,cap);
    if (ok && !profile_only) ok=save_record(&candidate,response,cap);
    memset(candidate.pass,0,sizeof(candidate.pass)); return ok;
}
bool jr_wifi_configure(const char *ssid,const char *password,const char *hostname,bool use_static_ip,
    const char *ip,const char *gw,const char *mask,const char *dns1,const char *dns2,char *response,unsigned cap) {
    return configure_record(false,ssid,password,hostname,use_static_ip,ip,gw,mask,dns1,dns2,response,cap);
}
bool jr_wifi_profile_save(const char *ssid,const char *password,const char *hostname,bool use_static_ip,
    const char *ip,const char *gw,const char *mask,const char *dns1,const char *dns2,char *response,unsigned cap) {
    return configure_record(true,ssid,password,hostname,use_static_ip,ip,gw,mask,dns1,dns2,response,cap);
}
bool jr_wifi_clear(char *response,unsigned cap) {
    /* A persistent tombstone also overrides compile-time credentials on reboot. */
    wifi_settings_t empty; defaults(&empty); empty.ssid[0]=0; memset(empty.pass,0,sizeof(empty.pass)); empty.use_static=0;
    return save_record(&empty,response,cap);
}
