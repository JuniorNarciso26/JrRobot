#include <stdio.h>
#include <string.h>
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
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
static esp_event_handler_instance_t wifi_monitor_handler, ip_monitor_handler;
static bool wifi_watch_set, ip_watch_set;
static uint16_t last_reason;
static uint32_t disconnect_events;
static uint32_t last_health_ms;

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

/* Event-only logging: do not scan, access NVS or allocate heap on Wi-Fi callbacks. */
static unsigned long uptime_ms(void) {
    return (unsigned long)(esp_timer_get_time()/1000ULL);
}

static void event_ssid(char out[33],const uint8_t *bytes,unsigned length) {
    unsigned n=length>32?32:length;
    memcpy(out,bytes,n);
    out[n]=0;
}

static void on_net_event(void *arg,esp_event_base_t base,int32_t id,void *data) {
    (void)arg;
    if(base==WIFI_EVENT && id==WIFI_EVENT_STA_CONNECTED){
        const wifi_event_sta_connected_t *ev=(const wifi_event_sta_connected_t *)data;
        if(!ev)return;
        char ssid[33];
        event_ssid(ssid,ev->ssid,ev->ssid_len);
        printf("JR_NET_ASSOC uptime_ms=%lu ssid=\"%s\" active_profile=\"%s\" bssid=%02X:%02X:%02X:%02X:%02X:%02X channel=%u authmode=%u\n",
               uptime_ms(),ssid,jr_wifi_ssid(),ev->bssid[0],ev->bssid[1],ev->bssid[2],
               ev->bssid[3],ev->bssid[4],ev->bssid[5],(unsigned)ev->channel,(unsigned)ev->authmode);
    } else if(base==WIFI_EVENT && id==WIFI_EVENT_STA_DISCONNECTED){
        const wifi_event_sta_disconnected_t *ev=(const wifi_event_sta_disconnected_t *)data;
        uint16_t reason=ev?ev->reason:0;
        char ssid[33]={0};
        if(ev)event_ssid(ssid,ev->ssid,ev->ssid_len);
        uint32_t count;
        portENTER_CRITICAL(&diag_lock);
        last_reason=reason;
        disconnect_events++;
        count=disconnect_events;
        portEXIT_CRITICAL(&diag_lock);
        printf("JR_NET_DIAG disconnect_event=%lu uptime_ms=%lu ssid=\"%s\" active_profile=\"%s\" bssid=%02X:%02X:%02X:%02X:%02X:%02X reason_code=%u reason=%s internal_free=%u internal_largest=%u psram_free=%u\n",
               (unsigned long)count,uptime_ms(),ssid,jr_wifi_ssid(),
               ev?ev->bssid[0]:0,ev?ev->bssid[1]:0,ev?ev->bssid[2]:0,
               ev?ev->bssid[3]:0,ev?ev->bssid[4]:0,ev?ev->bssid[5]:0,
               (unsigned)reason,reason_name(reason),
               (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),
               (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),
               (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    } else if(base==IP_EVENT && id==IP_EVENT_STA_GOT_IP){
        const ip_event_got_ip_t *ev=(const ip_event_got_ip_t *)data;
        if(!ev)return;
        printf("JR_NET_IP uptime_ms=%lu event=got_ip ip=" IPSTR " active_profile=\"%s\"\n",
               uptime_ms(),IP2STR(&ev->ip_info.ip),jr_wifi_ssid());
    } else if(base==IP_EVENT && id==IP_EVENT_STA_LOST_IP){
        printf("JR_NET_IP uptime_ms=%lu event=lost_ip active_profile=\"%s\"\n",
               uptime_ms(),jr_wifi_ssid());
    }
}

static void ensure_watch(void){
    if(!wifi_watch_set){
        esp_err_t err=esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,on_net_event,NULL,&wifi_monitor_handler);
        if(err==ESP_OK)wifi_watch_set=true;
        else printf("JR_NET_DIAG wifi_watch_error=%s\n",esp_err_to_name(err));
    }
    if(!ip_watch_set){
        esp_err_t err=esp_event_handler_instance_register(IP_EVENT,ESP_EVENT_ANY_ID,on_net_event,NULL,&ip_monitor_handler);
        if(err==ESP_OK)ip_watch_set=true;
        else printf("JR_NET_DIAG ip_watch_error=%s\n",esp_err_to_name(err));
    }
    watch_active=wifi_watch_set && ip_watch_set;
}

void jr_net_diag_watch(void) { ensure_watch(); }

/* One short line every 30 s, even when idle or no packets arrive.
 * The associated SSID/BSSID come from the driver, NOT from saved NVS.
 */
void jr_net_diag_poll(uint32_t now_ms){
    if((uint32_t)(now_ms-last_health_ms)<30000U)return;
    last_health_ms=now_ms;
    jr_wifi_status_t status={0};
    jr_wifi_get_status(&status);
    wifi_ap_record_t ap={0};
    bool radio=status.connected && esp_wifi_sta_get_ap_info(&ap)==ESP_OK;
    char actual_ssid[33]={0},bssid[18]="-";
    if(radio){
        event_ssid(actual_ssid,ap.ssid,32);
        snprintf(bssid,sizeof(bssid),"%02X:%02X:%02X:%02X:%02X:%02X",
                 ap.bssid[0],ap.bssid[1],ap.bssid[2],
                 ap.bssid[3],ap.bssid[4],ap.bssid[5]);
    }
    uint32_t count;uint16_t reason;
    portENTER_CRITICAL(&diag_lock);
    count=disconnect_events;reason=last_reason;
    portEXIT_CRITICAL(&diag_lock);
    printf("JR_NET_HEALTH uptime_ms=%lu connected=%d ip=%s active_profile=\"%s\" actual_ssid=\"%s\" bssid=%s rssi=%d channel=%u disconnects=%lu last_reason=%u internal_free=%u internal_largest=%u psram_free=%u\n",
           uptime_ms(),status.connected?1:0,status.ip,jr_wifi_ssid(),
           radio?actual_ssid:"-",bssid,radio?(int)ap.rssi:0,radio?(unsigned)ap.primary:0U,
           (unsigned long)count,(unsigned)reason,
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),
           (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
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
        snprintf(out->actual_ssid,sizeof(out->actual_ssid),"%.*s",32,(const char *)ap.ssid);
        snprintf(out->bssid,sizeof(out->bssid),"%02X:%02X:%02X:%02X:%02X:%02X",ap.bssid[0],ap.bssid[1],ap.bssid[2],ap.bssid[3],ap.bssid[4],ap.bssid[5]);
        out->rssi=ap.rssi;
        out->channel=ap.primary;
        out->authmode=(unsigned)ap.authmode;
    }

    printf("JR_NET_DIAG saved_found=%d saved_valid=%d source=%s ssid=\"%s\" host=%s mode=%s cfg_ip=%s gateway=%s mask=%s dns1=%s dns2=%s connected=%d network_ready=%d pending_restart=%d runtime_ip=%s last_error=%s watch=%d disconnect_events=%lu reason_code=%u reason=%s ap=%d bssid=%s channel=%u rssi=%d authmode=%u actual_ssid=\"%s\"\n",
        out->saved_record_found?1:0,out->saved_record_valid?1:0,out->source,out->ssid,out->hostname,out->static_mode?"IP_FIXO":"DHCP",out->configured_ip,out->gateway,out->mask,out->dns1,out->dns2,
        out->connected?1:0,out->network_ready?1:0,out->pending_restart?1:0,out->runtime_ip,esp_err_to_name(out->last_error),out->event_watch_active?1:0,(unsigned long)out->disconnect_events,
        (unsigned)out->last_disconnect_reason,out->disconnect_reason,out->ap_available?1:0,out->bssid,out->channel,out->rssi,out->authmode,out->actual_ssid);
}
