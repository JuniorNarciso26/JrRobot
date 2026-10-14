#include <stdio.h>
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_wifi.h"
#include "jr_mode_manager.h"
#include "jr_wifi.h"

#define SCAN_LIMIT 8

static void scan_memory(const char *stage) {
    printf("JR_WIFI_SCAN_HEAP stage=%s internal_free=%u internal_largest=%u\n",stage,
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
}

bool jr_wifi_scan(char *response,unsigned cap) {
    if (!response || !cap) return false;
    jr_wifi_status_t status;
    jr_wifi_get_status(&status);
    if (!status.network_ready) {
        snprintf(response,cap,"JR_WIFI_ERROR scan=not_started configure_o_wifi_e_reinicie");
        return false;
    }
    if (!status.connected) {
        snprintf(response,cap,"JR_WIFI_ERROR scan=not_connected aguarde_o_ip_ou_revise_a_rede");
        return false;
    }
    /* A saved boot change does not invalidate the station that is still connected. */
    if (jr_mode_get()!=JR_MODE_IDLE) {
        snprintf(response,cap,"JR_WIFI_ERROR scan=busy pare_live_e_autonomo_antes_da_busca");
        return false;
    }
    /* Only scan on request, using the existing station. No task or saved profile. */
    wifi_scan_config_t config={0};
    config.scan_type=WIFI_SCAN_TYPE_ACTIVE;
    config.scan_time.active.max=120;
    scan_memory("before");
    esp_err_t err=esp_wifi_scan_start(&config,true);
    if (err!=ESP_OK) {
        /* A failed start may belong to a scan/connect operation owned by the driver. */
        snprintf(response,cap,"JR_WIFI_ERROR scan=%s",esp_err_to_name(err));
        scan_memory("after");
        return false;
    }
    uint16_t total=0;
    err=esp_wifi_scan_get_ap_num(&total);
    unsigned shown=0;
    size_t used=0;
    if (err==ESP_OK) {
        int n=snprintf(response,cap,"JR_WIFI_SCAN {\"networks\":[");
        if (n<0 || (unsigned)n>=cap) err=ESP_ERR_INVALID_SIZE;
        else used=(size_t)n;
    }
    /* Read one record at a time: no extra array on the terminal stack or heap. */
    for (uint16_t i=0;err==ESP_OK && i<total && shown<SCAN_LIMIT;i++) {
        wifi_ap_record_t ap;
        err=esp_wifi_scan_get_ap_record(&ap);
        if (err!=ESP_OK) break;
        if (!ap.ssid[0]) continue;
        char encoded[97];
        static const char hex[]="0123456789ABCDEF";
        unsigned len=0;
        for (unsigned j=0;j<32 && ap.ssid[j];j++) {
            unsigned byte=ap.ssid[j];
            encoded[len++]='%'; encoded[len++]=hex[byte>>4]; encoded[len++]=hex[byte&15];
        }
        encoded[len]=0;
        int n=snprintf(response+used,cap-used,
            "%s{\"ssid_pct\":\"%s\",\"rssi\":%d,\"authmode\":%u,\"channel\":%u}",
            shown?",":"",encoded,(int)ap.rssi,(unsigned)ap.authmode,(unsigned)ap.primary);
        if (n<0 || (size_t)n>=cap-used) { err=ESP_ERR_INVALID_SIZE; break; }
        used+=(size_t)n; shown++;
    }
    /* Per-record reads release each entry; release every remaining driver entry too. */
    esp_err_t cleanup=esp_wifi_clear_ap_list();
    if (err==ESP_OK && cleanup!=ESP_OK) err=cleanup;
    if (err==ESP_OK) {
        int n=snprintf(response+used,cap-used,"],\"total\":%u,\"limited\":%s}",
            (unsigned)total,total>shown?"true":"false");
        if (n<0 || (size_t)n>=cap-used) err=ESP_ERR_INVALID_SIZE;
    }
    scan_memory("after");
    printf("JR_WIFI_SCAN_DONE total=%u shown=%u result=%s\n",(unsigned)total,shown,esp_err_to_name(err));
    if (err!=ESP_OK) snprintf(response,cap,"JR_WIFI_ERROR scan=%s",esp_err_to_name(err));
    return err==ESP_OK;
}
