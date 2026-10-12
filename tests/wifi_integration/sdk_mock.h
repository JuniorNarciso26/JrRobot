#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_INVALID_ARG 1
#define ESP_ERR_INVALID_STATE 2
#define ESP_ERR_NO_MEM 3
#define ESP_ERR_NVS_NOT_FOUND 4
#define ESP_ERR_INVALID_SIZE 5
#define ESP_ERR_INVALID_CRC 6
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
esp_err_t nvs_set_u8(nvs_handle_t,const char *,uint8_t);
esp_err_t nvs_commit(nvs_handle_t);
void nvs_close(nvs_handle_t);
typedef struct {uint32_t addr;} ipv4_t;
typedef struct {ipv4_t ip,netmask,gw;} esp_netif_ip_info_t;
typedef struct {esp_netif_ip_info_t ip_info;bool ip_changed;} ip_event_got_ip_t;
#define IPSTR "%u.%u.%u.%u"
#define IP2STR(p) ((p)->addr&255),(((p)->addr>>8)&255),(((p)->addr>>16)&255),(((p)->addr>>24)&255)
typedef struct {struct{uint8_t ssid[32],password[64];}sta;} wifi_config_t;
#define WIFI_IF_STA 0
esp_err_t esp_wifi_get_config(int,wifi_config_t *);
typedef void *esp_config_storage_t;
typedef struct {const char *nvs_namespace,*key_primary,*key_backup;} esp_config_storage_nvs_t;
typedef struct {esp_err_t(*encrypt)(void *,const uint8_t *,size_t,uint8_t *,size_t,size_t *);esp_err_t(*decrypt)(void *,const uint8_t *,size_t,uint8_t *,size_t,size_t *);void *ctx;} esp_config_crypto_ops_t;
esp_err_t esp_config_storage_init_nvs(esp_config_storage_nvs_t *,esp_config_storage_t *);
typedef void *esp_wifi_service_profile_mgr_t;
typedef struct {uint8_t flags,priority;char ssid[33],password[65];} esp_wifi_service_profile_t;
#define ESP_WIFI_SERVICE_PROFILE_FLAG_ENABLED 1
typedef struct {unsigned max_profiles;esp_config_storage_t storage;const esp_config_crypto_ops_t *crypto;size_t crypto_extra_size;} esp_wifi_service_profile_mgr_cfg_t;
esp_err_t esp_wifi_service_profile_mgr_init(const esp_wifi_service_profile_mgr_cfg_t *,esp_wifi_service_profile_mgr_t *);
esp_err_t esp_wifi_service_profile_mgr_count(esp_wifi_service_profile_mgr_t,uint8_t *);
esp_err_t esp_wifi_service_profile_mgr_add(esp_wifi_service_profile_mgr_t,const esp_wifi_service_profile_t *);
esp_err_t esp_wifi_service_profile_mgr_clear_all(esp_wifi_service_profile_mgr_t);
typedef struct esp_wifi_service {int unused;} esp_wifi_service_t;
typedef struct esp_service {int unused;} esp_service_t;
typedef void *esp_wifi_service_prov_t;
typedef struct {const char *name;esp_wifi_service_profile_mgr_t profile_manager;unsigned port;struct{const char *ssid;unsigned channel,max_connection;}softap;}esp_wifi_service_prov_http_config_t;
typedef struct {const char *name;esp_wifi_service_profile_mgr_t profile_manager;esp_wifi_service_prov_t *prov_list;size_t prov_num;void *selector_policy;}esp_wifi_service_config_t;
typedef struct {union{struct{esp_err_t err_code;}error;}data;}esp_wifi_service_prov_event_t;
enum {ESP_WIFI_SERVICE_EVENT_DISCONNECTED=102,ESP_WIFI_SERVICE_EVENT_STA_LOST_IP=112,ESP_WIFI_SERVICE_EVENT_STA_GOT_IP=111,ESP_WIFI_SERVICE_EVENT_PROV_CREDENTIAL_RECEIVED=107,ESP_WIFI_SERVICE_EVENT_PROV_STARTED=103,ESP_WIFI_SERVICE_EVENT_PROV_STOPPED=104,ESP_WIFI_SERVICE_EVENT_PROV_ERROR=109};
typedef struct {unsigned event_id;const void *payload;size_t payload_len;}adf_event_t;
typedef struct {unsigned event_id;void(*handler)(const adf_event_t *,void *);}adf_event_subscribe_info_t;
#define ADF_EVENT_SUBSCRIBE_INFO_DEFAULT() {0}
#define ADF_EVENT_ANY_ID 0
esp_err_t esp_wifi_service_create(const esp_wifi_service_config_t *,esp_wifi_service_t **);
esp_err_t esp_wifi_service_is_provisioning_running(esp_wifi_service_t *,bool *);
esp_err_t esp_wifi_service_start_provisioning(esp_wifi_service_t *);
esp_err_t esp_wifi_service_prov_http_create(const esp_wifi_service_prov_http_config_t *,esp_wifi_service_prov_t *);
esp_err_t esp_service_event_subscribe(esp_service_t *,const adf_event_subscribe_info_t *);
esp_err_t esp_service_start(esp_service_t *);
typedef int esp_aes_context;
#define ESP_AES_ENCRYPT 1
#define ESP_AES_DECRYPT 0
void esp_fill_random(void *,size_t);
void esp_aes_init(esp_aes_context *);
void esp_aes_free(esp_aes_context *);
int esp_aes_setkey(esp_aes_context *,const uint8_t *,unsigned);
int esp_aes_crypt_cbc(esp_aes_context *,int,size_t,uint8_t *,const uint8_t *,uint8_t *);
int esp_aes_crypt_ecb(esp_aes_context *,int,const uint8_t *,uint8_t *);
