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
#include "jr_mdns.h"
#include "esp_log.h"
#include "esp_config_storage.h"
#include "esp_wifi_service.h"
#include "esp_wifi_service_prov_http.h"
#include "aes/esp_aes.h"

#define CFG_SCHEMA 0x4a520201U
/* One versioned NVS value avoids mixed old/new fields after a failed write. */
typedef struct {
    uint32_t schema;
    uint8_t use_static;
    char ssid[33], pass[65], host[33], ip[16], gateway[16], mask[16], dns1[16], dns2[16];
} wifi_settings_t;
static wifi_settings_t active;
static bool prepared, start_attempted;
static esp_wifi_service_t *wifi_service;
static esp_wifi_service_profile_mgr_t profile_manager;
static esp_wifi_service_prov_t provisioning_agents[1];
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
    memset(c,0,sizeof(*c)); c->schema=CFG_SCHEMA; c->use_static=0;
    copy_checked(c->ssid,sizeof(c->ssid),JR_WIFI_SSID);
    copy_checked(c->pass,sizeof(c->pass),JR_WIFI_PASSWORD);
    copy_checked(c->host,sizeof(c->host),"jrbot");
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
    /* Preserve credentials, but migrate obsolete addressing metadata to DHCP.
     * Do not set import_pending: deleted profiles must not be reimported. */
    if (err==ESP_OK && valid_settings(&active,true) &&
        (active.use_static || strcmp(active.host,"jrbot"))) {
        active.use_static=0;
        copy_checked(active.host,sizeof(active.host),"jrbot");
        nvs_handle_t h;
        err=nvs_open("jr_wifi",NVS_READWRITE,&h);
        if (err==ESP_OK) {
            err=nvs_set_blob(h,"cfg_v2",&active,sizeof(active));
            if (err==ESP_OK) err=nvs_commit(h);
            nvs_close(h);
        }
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


/* Keep the proven experiment's store/format so flashing the candidate retains
 * profiles configured by phone. This is demo application-layer encryption,
 * not hardware-backed NVS protection; do not treat the published key as a secret.
 */
static esp_config_storage_nvs_t profile_store = {
    .nvs_namespace = "wifi_svc_ex",
    .key_primary = "prof_p",
    .key_backup = "prof_b",
};
static esp_config_storage_t profile_storage;
#define WIFI_SVC_EX_AES_KEY_LEN_BYTES    (16u)
#define WIFI_SVC_EX_AES_BLOCK_SIZE       (16u)
#define WIFI_SVC_EX_AES_BLOB_OVERHEAD    (WIFI_SVC_EX_AES_BLOCK_SIZE * 2u)  // IV + worst-case PKCS7 padding
#define WIFI_SVC_EX_PROFILE_MAX_NUM      (8u)
#define WIFI_SVC_EX_STA_LISTEN_INTERVAL  (3u)

static esp_err_t wifi_svc_ex_profile_encrypt(void *ctx, const uint8_t *in, size_t in_len, uint8_t *out, size_t out_size, size_t *out_len)
{
    const uint8_t *key = ctx;
    if (!in || !out || !out_len || !key) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t padded_len = ((in_len / WIFI_SVC_EX_AES_BLOCK_SIZE) + 1u) * WIFI_SVC_EX_AES_BLOCK_SIZE;
    size_t total_len = WIFI_SVC_EX_AES_BLOCK_SIZE + padded_len;  // [IV(16B) | CIPHERTEXT]
    if (out_size < total_len) {
        return ESP_ERR_NO_MEM;
    }

    esp_fill_random(out, WIFI_SVC_EX_AES_BLOCK_SIZE);
    uint8_t iv[WIFI_SVC_EX_AES_BLOCK_SIZE];
    memcpy(iv, out, sizeof(iv));

    memcpy(out + WIFI_SVC_EX_AES_BLOCK_SIZE, in, in_len);
    uint8_t pad = (uint8_t)(padded_len - in_len);
    memset(out + WIFI_SVC_EX_AES_BLOCK_SIZE + in_len, (int)pad, pad);

    esp_aes_context aes;
    esp_aes_init(&aes);
    int aes_ret = esp_aes_setkey(&aes, key, (unsigned int)(WIFI_SVC_EX_AES_KEY_LEN_BYTES * 8u));
    if (aes_ret == 0) {
        aes_ret = esp_aes_crypt_cbc(&aes, ESP_AES_ENCRYPT, padded_len, iv, out + WIFI_SVC_EX_AES_BLOCK_SIZE,
                                    out + WIFI_SVC_EX_AES_BLOCK_SIZE);
    }
    esp_aes_free(&aes);
    if (aes_ret != 0) {
        return ESP_FAIL;
    }

    *out_len = total_len;
    return ESP_OK;
}

static esp_err_t wifi_svc_ex_profile_decrypt(void *ctx, const uint8_t *in, size_t in_len, uint8_t *out, size_t out_size, size_t *out_len)
{
    const uint8_t *key = ctx;
    if (!in || !out || !out_len || !key) {
        return ESP_ERR_INVALID_ARG;
    }
    if (in_len < (WIFI_SVC_EX_AES_BLOCK_SIZE * 2u)) {
        return ESP_ERR_INVALID_SIZE;
    }

    size_t cipher_len = in_len - WIFI_SVC_EX_AES_BLOCK_SIZE;
    if ((cipher_len % WIFI_SVC_EX_AES_BLOCK_SIZE) != 0u) {
        return ESP_ERR_INVALID_SIZE;
    }
    if (out_size < (cipher_len - WIFI_SVC_EX_AES_BLOCK_SIZE)) {
        return ESP_ERR_NO_MEM;
    }

    const uint8_t *cipher = in + WIFI_SVC_EX_AES_BLOCK_SIZE;
    uint8_t prev[WIFI_SVC_EX_AES_BLOCK_SIZE];
    memcpy(prev, in, sizeof(prev));

    esp_aes_context aes;
    esp_aes_init(&aes);
    int aes_ret = esp_aes_setkey(&aes, key, (unsigned int)(WIFI_SVC_EX_AES_KEY_LEN_BYTES * 8u));
    size_t out_pos = 0;
    uint8_t plain_block[WIFI_SVC_EX_AES_BLOCK_SIZE];
    uint8_t last_block[WIFI_SVC_EX_AES_BLOCK_SIZE];

    if (aes_ret == 0) {
        for (size_t off = 0; off < cipher_len; off += WIFI_SVC_EX_AES_BLOCK_SIZE) {
            uint8_t dec_block[WIFI_SVC_EX_AES_BLOCK_SIZE];
            aes_ret = esp_aes_crypt_ecb(&aes, ESP_AES_DECRYPT, cipher + off, dec_block);
            if (aes_ret != 0) {
                break;
            }

            for (size_t i = 0; i < WIFI_SVC_EX_AES_BLOCK_SIZE; ++i) {
                plain_block[i] = dec_block[i] ^ prev[i];
            }
            memcpy(prev, cipher + off, WIFI_SVC_EX_AES_BLOCK_SIZE);

            if (off + WIFI_SVC_EX_AES_BLOCK_SIZE < cipher_len) {
                if (out_pos + WIFI_SVC_EX_AES_BLOCK_SIZE > out_size) {
                    aes_ret = -1;
                    break;
                }
                memcpy(out + out_pos, plain_block, WIFI_SVC_EX_AES_BLOCK_SIZE);
                out_pos += WIFI_SVC_EX_AES_BLOCK_SIZE;
            } else {
                memcpy(last_block, plain_block, WIFI_SVC_EX_AES_BLOCK_SIZE);
            }
        }
    }
    esp_aes_free(&aes);
    if (aes_ret != 0) {
        return ESP_FAIL;
    }

    uint8_t pad = last_block[WIFI_SVC_EX_AES_BLOCK_SIZE - 1u];
    if ((pad == 0u) || (pad > WIFI_SVC_EX_AES_BLOCK_SIZE)) {
        return ESP_ERR_INVALID_CRC;
    }
    for (size_t i = 0; i < pad; ++i) {
        if (last_block[WIFI_SVC_EX_AES_BLOCK_SIZE - 1u - i] != pad) {
            return ESP_ERR_INVALID_CRC;
        }
    }

    size_t last_data = WIFI_SVC_EX_AES_BLOCK_SIZE - pad;
    if (out_pos + last_data > out_size) {
        return ESP_ERR_NO_MEM;
    }
    memcpy(out + out_pos, last_block, last_data);
    out_pos += last_data;
    *out_len = out_pos;
    return ESP_OK;
}

static uint8_t s_profile_crypto_key[WIFI_SVC_EX_AES_KEY_LEN_BYTES] = {
    0x42, 0x10, 0x7A, 0xE3, 0x09, 0x5D, 0xBC, 0x21,
    0x6F, 0x80, 0x11, 0xCA, 0x34, 0x9B, 0xDE, 0x57,
};

static const esp_config_crypto_ops_t s_profile_crypto_ops = {
    .encrypt = wifi_svc_ex_profile_encrypt,
    .decrypt = wifi_svc_ex_profile_decrypt,
    .ctx     = s_profile_crypto_key,
};


bool jr_wifi_is_provisioning(void) {
    bool running=false;
    if (wifi_service) (void)esp_wifi_service_is_provisioning_running(wifi_service,&running);
    return running;
}
esp_err_t jr_wifi_begin_provisioning(void) {
    if (!wifi_service) return ESP_ERR_INVALID_STATE;
    return esp_wifi_service_start_provisioning(wifi_service);
}

static void wifi_service_event(const adf_event_t *event,void *ctx) {
    (void)ctx;
    if (event->event_id==ESP_WIFI_SERVICE_EVENT_DISCONNECTED ||
        event->event_id==ESP_WIFI_SERVICE_EVENT_STA_LOST_IP) {
        portENTER_CRITICAL(&state_lock);
        state.connected=false;
        memcpy(state.ip,"sem wifi",9);
        portEXIT_CRITICAL(&state_lock);
    } else if (event->event_id==ESP_WIFI_SERVICE_EVENT_STA_GOT_IP &&
               event->payload && event->payload_len>=sizeof(ip_event_got_ip_t)) {
        const ip_event_got_ip_t *info=event->payload;
        char ip[16]={0};
        snprintf(ip,sizeof(ip),IPSTR,IP2STR(&info->ip_info.ip));
        wifi_config_t cfg={0};
        if (esp_wifi_get_config(WIFI_IF_STA,&cfg)==ESP_OK) {
            snprintf(active.ssid,sizeof(active.ssid),"%.*s",32,(const char *)cfg.sta.ssid);
        }
        portENTER_CRITICAL(&state_lock);
        memcpy(state.ip,ip,sizeof(ip)); state.connected=true;
        state.configured=true; state.last_error=ESP_OK;
        portEXIT_CRITICAL(&state_lock);
        esp_err_t mdns_err=jr_mdns_start();
        printf("JR_WIFI conectado DHCP ip=%s nome=jrbot.local mdns=%s\n",ip,esp_err_to_name(mdns_err));
    } else if (event->event_id==ESP_WIFI_SERVICE_EVENT_PROV_CREDENTIAL_RECEIVED) {
        portENTER_CRITICAL(&state_lock);
        state.configured=true; state.pending_restart=false;
        portEXIT_CRITICAL(&state_lock);
        puts("JR_WIFI configuracao_salva portal_encerramento_automatico");
    } else if (event->event_id==ESP_WIFI_SERVICE_EVENT_PROV_STARTED) {
        puts("JR_WIFI_SETUP rede=JrBot-Config url=http://192.168.4.1");
    } else if (event->event_id==ESP_WIFI_SERVICE_EVENT_PROV_STOPPED) {
        puts("JR_WIFI_SETUP encerrado portal=http://jrbot.local");
    } else if (event->event_id==ESP_WIFI_SERVICE_EVENT_PROV_ERROR &&
               event->payload && event->payload_len>=sizeof(esp_wifi_service_prov_event_t)) {
        const esp_wifi_service_prov_event_t *info=event->payload;
        set_error(info->data.error.err_code);
        printf("JR_WIFI_SETUP erro=%s tente_novamente\n",esp_err_to_name(info->data.error.err_code));
    }
    /* Do not stop HTTP provisioning on phone disconnect: an old saved profile
     * does not establish success of the current credential attempt.
     */
}

void jr_wifi_start(void) {
    if (start_attempted) return;
    start_attempted=true;
    jr_wifi_prepare();
    esp_err_t err=ensure_nvs();
    if (err!=ESP_OK) goto fail;
    err=esp_config_storage_init_nvs(&profile_store,&profile_storage);
    if (err!=ESP_OK) goto fail;
    esp_wifi_service_profile_mgr_cfg_t profile_cfg={
        .max_profiles=WIFI_SVC_EX_PROFILE_MAX_NUM,
        .storage=profile_storage,
        .crypto=&s_profile_crypto_ops,
        .crypto_extra_size=WIFI_SVC_EX_AES_BLOB_OVERHEAD,
    };
    err=esp_wifi_service_profile_mgr_init(&profile_cfg,&profile_manager);
    if (err!=ESP_OK) goto fail;
    uint8_t count=0,import_pending=0,migration_done=0;
    nvs_handle_t h;
    if (nvs_open("jr_wifi",NVS_READONLY,&h)==ESP_OK) {
        (void)nvs_get_u8(h,"import_pending",&import_pending);
        (void)nvs_get_u8(h,"migration_done",&migration_done);
        nvs_close(h);
    }
    err=esp_wifi_service_profile_mgr_count(profile_manager,&count);
    if (err!=ESP_OK) goto fail;
    if (import_pending || (!migration_done && !count && active.ssid[0])) {
        if (active.ssid[0]) {
            esp_wifi_service_profile_t profile={
                .flags=ESP_WIFI_SERVICE_PROFILE_FLAG_ENABLED,.priority=10,
            };
            copy_checked(profile.ssid,sizeof(profile.ssid),active.ssid);
            copy_checked(profile.password,sizeof(profile.password),active.pass);
            err=esp_wifi_service_profile_mgr_add(profile_manager,&profile);
        } else {
            err=esp_wifi_service_profile_mgr_clear_all(profile_manager);
        }
        if (err!=ESP_OK) goto fail;
        if (import_pending) {
            err=nvs_open("jr_wifi",NVS_READWRITE,&h);
            if (err!=ESP_OK) goto fail;
            err=nvs_set_u8(h,"import_pending",0);
            if (err==ESP_OK) err=nvs_commit(h);
            nvs_close(h);
            if (err!=ESP_OK) goto fail;
        }
        err=esp_wifi_service_profile_mgr_count(profile_manager,&count);
        if (err!=ESP_OK) goto fail;
    }
    if (!migration_done) {
        err=nvs_open("jr_wifi",NVS_READWRITE,&h);
        if (err!=ESP_OK) goto fail;
        err=nvs_set_u8(h,"migration_done",1);
        if (err==ESP_OK) err=nvs_commit(h);
        nvs_close(h);
        if (err!=ESP_OK) goto fail;
    }
    esp_wifi_service_prov_http_config_t http_cfg={
        .name="http",.profile_manager=profile_manager,.port=8080,
        .softap={.ssid="JrBot-Config",.channel=1,.max_connection=2},
    };
    err=esp_wifi_service_prov_http_create(&http_cfg,&provisioning_agents[0]);
    if (err!=ESP_OK) goto fail;
    esp_wifi_service_config_t cfg={
        .name="jrbot_wifi",.profile_manager=profile_manager,
        .prov_list=provisioning_agents,.prov_num=1,.selector_policy=NULL,
    };
    err=esp_wifi_service_create(&cfg,&wifi_service);
    if (err!=ESP_OK) goto fail;
    adf_event_subscribe_info_t sub=ADF_EVENT_SUBSCRIBE_INFO_DEFAULT();
    sub.event_id=ADF_EVENT_ANY_ID;
    sub.handler=wifi_service_event;
    err=esp_service_event_subscribe((esp_service_t *)wifi_service,&sub);
    if (err!=ESP_OK) goto fail;
    err=esp_service_start((esp_service_t *)wifi_service);
    if (err!=ESP_OK) goto fail;
    /* Start after the service has created its station interface. */
    esp_err_t mdns_err=jr_mdns_start();
    if (mdns_err!=ESP_OK) printf("JR_MDNS_ERROR %s WiFi_acessivel_por_IP\n",esp_err_to_name(mdns_err));
    portENTER_CRITICAL(&state_lock);
    state.network_ready=true; state.configured=count!=0; state.pending_restart=false; state.last_error=ESP_OK;
    portEXIT_CRITICAL(&state_lock);
    puts("JR_WIFI servico_iniciado DHCP nome=jrbot.local");
    return;
fail:
    set_error(err);
    printf("JR_WIFI_ERROR start=%s diagnostico_serial_disponivel\n",esp_err_to_name(err));
}
static bool save_record(const wifi_settings_t *c,char *response,unsigned cap) {
    esp_err_t err=ensure_nvs(); nvs_handle_t h;
    if (err==ESP_OK) {
        err=nvs_open("jr_wifi",NVS_READWRITE,&h);
        if (err==ESP_OK) {
            err=nvs_set_blob(h,"cfg_v2",c,sizeof(*c));
            if (err==ESP_OK) err=nvs_set_u8(h,"import_pending",1);
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
    (void)hostname; /* The local address is always jrbot.local. */
    if (use_static_ip) { snprintf(response,cap,"JR_WIFI_ERROR use_DHCP_com_jrbot.local"); return false; }
    wifi_settings_t candidate; defaults(&candidate); candidate.use_static=use_static_ip?1:0;
    bool valid=copy_checked(candidate.ssid,sizeof(candidate.ssid),ssid) &&
        copy_checked(candidate.pass,sizeof(candidate.pass),password) &&
        copy_checked(candidate.host,sizeof(candidate.host),"jrbot") &&
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
