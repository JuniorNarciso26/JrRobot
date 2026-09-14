#include <stdio.h>
#include <string.h>
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "jr_net_diag.h"
#include "jr_wifi.h"

#define JR_NET_CFG_SCHEMA 0x4a520201U

typedef struct {
    uint32_t schema;
    uint8_t static_mode;
    char ssid[33];
    char hidden[65];
    char hostname[33];
    char ip[16];
    char gateway[16];
    char mask[16];
    char dns1[16];
    char dns2[16];
} jr_net_record_t;

static portMUX_TYPE diag_lock=portMUX_INITIALIZER_UNLOCKED;
static bool watch_active;
static esp_event_handler_instance_t disconnect_handler;
static uint16_t last_reason;
static uint32_t disconnect_events;

static bool terminated(const char *s,size_t cap){ return memchr(s,0,cap)!=NULL; }
static void copy_text(char *dst,size_t cap,const char *src){ if(!cap)return; if(!src)src=""; snprintf(dst,cap,"%s",src); }

static const char *reason_name(uint16_t reason){
    switch(reason){
        case 0:return "NONE";
        case 2:return "AUTH_EXPIRE";
        case 15:return "4WAY_HANDSHAKE_TIMEOUT";
        case 23:return "8021X_AUTH_FAILED";
        case 200:return "BEACON_TIMEOUT";
        case 201:return "NO_AP_FOUND";
        case 202:return "AUTH_FAIL";
        case 203:return "ASSOC_FAIL";
        case 204:return "HANDSHAKE_TIMEOUT";
        case 205:return "CONNECTION_FAIL";
        case 206:return "AP_TSF_RESET";
        default:return "OTHER";
    }
}

static void on_wifi_event(void *arg,esp_event_base_t base,int32_t id,void *data){
    (void)arg;
    if(base!=WIFI_EVENT||id!=WIFI_EVENT_STA_DISCONNECTED)return;
    const wifi_event_sta_disconnected_t *event=(const wifi_event_sta_disconnected_t *)data;
    uint16_t reason=event?event->reason:0;
    uint32_t count;
    portENTER_CRITICAL(&diag_lock);
    last_reason=reason;
    disconnect_events++;
    count=disconnect_events;
    portEXIT_CRITICAL(&diag_lock);
    printf("JR_NET_DIAG disconnect_event=%lu reason_code=%u reason=%s\n",(unsigned long)count,(unsigned)reason,reason_name(reason));
}

static void ensure_watch(void){
    if(watch_active)return;
    esp_err_t err=esp_event_handler_instance_register(WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,on_wifi_event,NULL,&disconnect_handler);
    if(err==ESP_OK)watch_active=true;
}

static bool record_valid(const jr_net_record_t *r){
    return r&&r->schema==JR_NET_CFG_SCHEMA&&r->static_mode<=1&&
        terminated(r->ssid,sizeof(r->ssid))&&terminated(r->hostname,sizeof(r->hostname))&&
        terminated(r->ip,sizeof(r->ip))&&terminated(r->gateway,sizeof(r->gateway))&&
        terminated(r->mask,sizeof(r->mask))&&terminated(r->dns1,sizeof(r->dns1))&&terminated(r->dns2,sizeof(r->dns2));
}

static bool read_saved(jr_net_diag_t *out){
    esp_err_t err=nvs_flash_init();
    if(err!=ESP_OK)return false;
    nvs_handle_t h;
    err=nvs_open("jr_wifi",NVS_READONLY,&h);
    if(err!=ESP_OK)return false;
    jr_net_record_t r={0};
    size_t n=sizeof(r);
    err=nvs_get_blob(h,"cfg_v2",&r,&n);
    if(err==ESP_OK&&n==sizeof(r)){
        out->saved_record_found=true;
        out->saved_record_valid=record_valid(&r);
        copy_text(out->source,sizeof(out->source),"nvs_v2");
        if(out->saved_record_valid){
            copy_text(out->ssid,sizeof(out->ssid),r.ssid);
            copy_text(out->hostname,sizeof(out->hostname),r.hostname);
            out->static_mode=r.static_mode!=0;
            copy_text(out->configured_ip,sizeof(out->configured_ip),r.ip);
            copy_text(out->gateway,sizeof(out->gateway),r.gateway);
            copy_text(out->mask,sizeof(out->mask),r.mask);
            copy_text(out->dns1,sizeof(out->dns1),r.dns1);
            copy_text(out->dns2,sizeof(out->dns2),r.dns2);
        }
        nvs_close(h);
        return out->saved_record_valid;
    }

    size_t sn=sizeof(out->ssid);
    if(nvs_get_str(h,"ssid",out->ssid,&sn)==ESP_OK){
        out->saved_record_found=true;
        out->saved_record_valid=true;
        copy_text(out->source,sizeof(out->source),"legacy");
        size_t hn=sizeof(out->hostname); if(nvs_get_str(h,"host",out->hostname,&hn)!=ESP_OK)copy_text(out->hostname,sizeof(out->hostname),"jrbot");
        size_t ipn=sizeof(out->configured_ip); nvs_get_str(h,"ip",out->configured_ip,&ipn);
        size_t gn=sizeof(out->gateway); nvs_get_str(h,"gw",out->gateway,&gn);
        size_t mn=sizeof(out->mask); nvs_get_str(h,"mask",out->mask,&mn);
        size_t d1n=sizeof(out->dns1); nvs_get_str(h,"dns1",out->dns1,&d1n);
        size_t d2n=sizeof(out->dns2); nvs_get_str(h,"dns2",out->dns2,&d2n);
        uint8_t sm=0; nvs_get_u8(h,"static",&sm); out->static_mode=sm!=0;
        nvs_close(h);
        return true;
    }
    nvs_close(h);
    copy_text(out->source,sizeof(out->source),"none");
    return false;
}

void jr_net_diag_snapshot(jr_net_diag_t *out){
    if(!out)return;
    memset(out,0,sizeof(*out));
    copy_text(out->runtime_ip,sizeof(out->runtime_ip),"sem wifi");
    copy_text(out->bssid,sizeof(out->bssid),"-");
    read_saved(out);
    ensure_watch();

    jr_wifi_status_t status={0};
    jr_wifi_get_status(&status);
    out->connected=status.connected;
    out->network_ready=status.network_ready;
    out->pending_restart=status.pending_restart;
    copy_text(out->runtime_ip,sizeof(out->runtime_ip),status.ip);
    out->last_error=status.last_error;

    portENTER_CRITICAL(&diag_lock);
    out->event_watch_active=watch_active;
    out->last_disconnect_reason=last_reason;
    out->disconnect_events=disconnect_events;
    portEXIT_CRITICAL(&diag_lock);
    copy_text(out->disconnect_reason,sizeof(out->disconnect_reason),reason_name(out->last_disconnect_reason));

    wifi_ap_record_t ap={0};
    if(esp_wifi_sta_get_ap_info(&ap)==ESP_OK){
        out->ap_available=true;
        snprintf(out->bssid,sizeof(out->bssid),"%02X:%02X:%02X:%02X:%02X:%02X",ap.bssid[0],ap.bssid[1],ap.bssid[2],ap.bssid[3],ap.bssid[4],ap.bssid[5]);
        out->rssi=ap.rssi;
        out->channel=ap.primary;
        out->authmode=(unsigned)ap.authmode;
    }

    printf("JR_NET_DIAG saved_found=%d saved_valid=%d source=%s ssid=\"%s\" host=%s mode=%s cfg_ip=%s gateway=%s mask=%s dns1=%s dns2=%s connected=%d network_ready=%d pending_restart=%d runtime_ip=%s last_error=%s watch=%d disconnect_events=%lu reason_code=%u reason=%s ap=%d bssid=%s channel=%u rssi=%d authmode=%u\n",
        out->saved_record_found?1:0,out->saved_record_valid?1:0,out->source,out->ssid,out->hostname,out->static_mode?"IP_FIXO":"DHCP",out->configured_ip,out->gateway,out->mask,out->dns1,out->dns2,
        out->connected?1:0,out->network_ready?1:0,out->pending_restart?1:0,out->runtime_ip,esp_err_to_name(out->last_error),out->event_watch_active?1:0,(unsigned long)out->disconnect_events,
        (unsigned)out->last_disconnect_reason,out->disconnect_reason,out->ap_available?1:0,out->bssid,out->channel,out->rssi,out->authmode);
}
