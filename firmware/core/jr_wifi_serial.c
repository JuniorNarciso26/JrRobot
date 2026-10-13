#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_service.h"
#include "jr_wifi.h"
#include "jr_wifi_serial.h"

#define SCAN_LIMIT 12
typedef struct { char ssid[33]; int rssi,auth; } scan_row_t;
static scan_row_t rows[SCAN_LIMIT];
static unsigned row_count;
static bool scan_pending;
static esp_err_t scan_error;
static portMUX_TYPE scan_lock=portMUX_INITIALIZER_UNLOCKED;

/* ASCII wire encoding prevents names with spaces, delimiters or accents from
 * corrupting Serial records. No saved passwords are returned. */
static void encode_ssid(const char *s,char out[97]) {
    static const char hex[]="0123456789ABCDEF";
    unsigned n=0;
    for (unsigned i=0;i<32 && s[i];i++) {
        unsigned char ch=(unsigned char)s[i];
        if ((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||(ch>='0'&&ch<='9')||ch=='-'||ch=='_'||ch=='.') out[n++]=(char)ch;
        else {out[n++]='%';out[n++]=hex[ch>>4];out[n++]=hex[ch&15];}
    }
    out[n]=0;
}
static bool reply_error(esp_err_t err,char *out,size_t cap) {
    snprintf(out,cap,"JR_WIFI_ERROR %s",esp_err_to_name(err));
    return false;
}
static void scan_done(esp_err_t err,uint16_t count,const wifi_ap_record_t *records,void *ctx) {
    (void)ctx;
    portENTER_CRITICAL(&scan_lock);
    row_count=0;
    if (err==ESP_OK && records) {
        for (unsigned i=0;i<count && row_count<SCAN_LIMIT;i++) {
            if (!records[i].ssid[0]) continue;
            scan_row_t *r=&rows[row_count++];
            memcpy(r->ssid,records[i].ssid,32);r->ssid[32]=0;
            r->rssi=records[i].rssi;r->auth=records[i].authmode;
        }
    }
    scan_error=err;scan_pending=false;
    portEXIT_CRITICAL(&scan_lock);
}
bool jr_wifi_serial_scan(bool start,char *out,size_t cap) {
    esp_wifi_service_t *service=jr_wifi_service_handle();
    if (!service) return reply_error(ESP_ERR_INVALID_STATE,out,cap);
    if (start) {
        portENTER_CRITICAL(&scan_lock);
        bool pending=scan_pending;
        if (!pending) {scan_pending=true;scan_error=ESP_OK;}
        portEXIT_CRITICAL(&scan_lock);
        if (!pending) {
            esp_wifi_service_scan_handle_t agent=NULL;
            esp_err_t err=esp_wifi_service_get_scan_handle(service,&agent);
            if (err==ESP_OK) err=esp_wifi_service_scan_request(agent,NULL,scan_done,NULL);
            if (err!=ESP_OK) {scan_done(err,0,NULL,NULL);return reply_error(err,out,cap);}
        }
    }
    scan_row_t copy[SCAN_LIMIT];unsigned count;bool pending;esp_err_t err;
    portENTER_CRITICAL(&scan_lock);
    count=row_count;pending=scan_pending;err=scan_error;memcpy(copy,rows,sizeof(copy));
    portEXIT_CRITICAL(&scan_lock);
    if (err!=ESP_OK) return reply_error(err,out,cap);
    int n=snprintf(out,cap,"JR_WIFI_NETWORKS pending=%d count=%u data=",pending?1:0,count);
    if (n<0 || (size_t)n>=cap) return reply_error(ESP_ERR_NO_MEM,out,cap);
    size_t used=(size_t)n;
    for (unsigned i=0;i<count;i++) {
        char encoded[97];encode_ssid(copy[i].ssid,encoded);
        n=snprintf(out+used,cap-used,"%s%s,%d,%d",i?";":"",encoded,copy[i].rssi,copy[i].auth);
        if (n<0 || (size_t)n>=cap-used) return reply_error(ESP_ERR_NO_MEM,out,cap);
        used+=(size_t)n;
    }
    return true;
}
typedef struct {char *out;size_t cap,used;unsigned count;bool failed;} list_ctx_t;
static bool profile_row(const esp_wifi_service_profile_t *p,void *data) {
    list_ctx_t *ctx=data;char encoded[97];encode_ssid(p->ssid,encoded);
    int n=snprintf(ctx->out+ctx->used,ctx->cap-ctx->used,"%s%s,%u,%u",ctx->count?";":"",encoded,
        (p->flags&ESP_WIFI_SERVICE_PROFILE_FLAG_ENABLED)?1:0,p->priority);
    if (n<0 || (size_t)n>=ctx->cap-ctx->used) {ctx->failed=true;return false;}
    ctx->used+=(size_t)n;ctx->count++;return true;
}
bool jr_wifi_serial_profiles(char *out,size_t cap) {
    esp_wifi_service_profile_mgr_t mgr=NULL;
    esp_err_t err=esp_wifi_service_get_profile_manager(jr_wifi_service_handle(),&mgr);
    if (err!=ESP_OK) return reply_error(err,out,cap);
    wifi_ap_record_t ap={0};char connected[97]="";
    if (esp_wifi_sta_get_ap_info(&ap)==ESP_OK) encode_ssid((const char *)ap.ssid,connected);
    int n=snprintf(out,cap,"JR_WIFI_PROFILES connected=%s data=",connected);
    if (n<0 || (size_t)n>=cap) return reply_error(ESP_ERR_NO_MEM,out,cap);
    list_ctx_t ctx={.out=out,.cap=cap,.used=(size_t)n};
    err=esp_wifi_service_profile_mgr_foreach(mgr,profile_row,&ctx);
    if (err!=ESP_OK || ctx.failed) return reply_error(err!=ESP_OK?err:ESP_ERR_NO_MEM,out,cap);
    return true;
}
bool jr_wifi_serial_delete(const char *ssid,char *out,size_t cap) {
    esp_wifi_service_profile_mgr_t mgr=NULL;
    esp_err_t err=esp_wifi_service_get_profile_manager(jr_wifi_service_handle(),&mgr);
    if (err==ESP_OK) err=esp_wifi_service_profile_mgr_delete(mgr,ssid);
    if (err!=ESP_OK) return reply_error(err,out,cap);
    snprintf(out,cap,"JR_WIFI_DELETED");
    /* Removing a profile does not forcibly drop the current connection. */
    return true;
}
bool jr_wifi_serial_join(const char *ssid,const char *password,char *out,size_t cap) {
    size_t len=strlen(password);
    if (!ssid[0] || strlen(ssid)>32 || len>64 || (len && len<8)) return reply_error(ESP_ERR_INVALID_ARG,out,cap);
    if (len==64) for (size_t i=0;i<len;i++) if (!isxdigit((unsigned char)password[i])) return reply_error(ESP_ERR_INVALID_ARG,out,cap);
    esp_err_t err=jr_wifi_serial_apply_credentials(ssid,password);
    if (err!=ESP_OK) return reply_error(err,out,cap);
    snprintf(out,cap,"JR_WIFI_CONNECTED host=jrbot.local ip=%s",jr_wifi_ip());
    return true;
}
