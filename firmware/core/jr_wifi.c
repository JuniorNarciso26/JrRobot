#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "lwip/inet.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "jr_config.h"
#include "jr_wifi.h"

static bool wifi_started = false;
static bool wifi_connected = false;
static char wifi_ip[16] = "sem wifi";
static char cfg_ssid[33] = "";
static char cfg_pass[65] = "";
static char cfg_host[33] = "jrbot";
static bool cfg_static = false;
static char cfg_ip[16] = "192.168.0.50";
static char cfg_gateway[16] = "192.168.0.1";
static char cfg_subnet[16] = "255.255.255.0";
static char cfg_dns1[16] = "8.8.8.8";
static char cfg_dns2[16] = "8.8.4.4";

static void copy_field(char *dst, size_t dst_len, const char *src, const char *fallback) {
    const char *value = (src && src[0]) ? src : fallback;
    if (!value) value = "";
    snprintf(dst, dst_len, "%s", value);
}

static void nvs_get_string(nvs_handle_t nvs, const char *key, char *dst, size_t dst_len) {
    size_t len = dst_len;
    if (nvs_get_str(nvs, key, dst, &len) != ESP_OK) dst[0] = '\0';
}

static esp_err_t ensure_nvs(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

static void wifi_load_config(void) {
    copy_field(cfg_ssid, sizeof(cfg_ssid), JR_WIFI_SSID, "");
    copy_field(cfg_pass, sizeof(cfg_pass), JR_WIFI_PASSWORD, "");
    copy_field(cfg_host, sizeof(cfg_host), JR_WIFI_HOSTNAME, "jrbot");
    cfg_static = JR_WIFI_USE_STATIC_IP ? true : false;
    copy_field(cfg_ip, sizeof(cfg_ip), JR_WIFI_STATIC_IP, "192.168.0.50");
    copy_field(cfg_gateway, sizeof(cfg_gateway), JR_WIFI_GATEWAY, "192.168.0.1");
    copy_field(cfg_subnet, sizeof(cfg_subnet), JR_WIFI_SUBNET, "255.255.255.0");
    copy_field(cfg_dns1, sizeof(cfg_dns1), JR_WIFI_DNS1, "8.8.8.8");
    copy_field(cfg_dns2, sizeof(cfg_dns2), JR_WIFI_DNS2, "8.8.4.4");

    nvs_handle_t nvs;
    if (ensure_nvs() != ESP_OK) return;
    if (nvs_open("jr_wifi", NVS_READONLY, &nvs) == ESP_OK) {
        char tmp[65];
        nvs_get_string(nvs, "ssid", tmp, sizeof(tmp)); if (tmp[0]) copy_field(cfg_ssid, sizeof(cfg_ssid), tmp, "");
        nvs_get_string(nvs, "pass", tmp, sizeof(tmp)); if (tmp[0]) copy_field(cfg_pass, sizeof(cfg_pass), tmp, "");
        nvs_get_string(nvs, "host", tmp, sizeof(tmp)); if (tmp[0]) copy_field(cfg_host, sizeof(cfg_host), tmp, "jrbot");
        nvs_get_string(nvs, "ip", tmp, sizeof(tmp)); if (tmp[0]) copy_field(cfg_ip, sizeof(cfg_ip), tmp, "192.168.0.50");
        nvs_get_string(nvs, "gw", tmp, sizeof(tmp)); if (tmp[0]) copy_field(cfg_gateway, sizeof(cfg_gateway), tmp, "192.168.0.1");
        nvs_get_string(nvs, "mask", tmp, sizeof(tmp)); if (tmp[0]) copy_field(cfg_subnet, sizeof(cfg_subnet), tmp, "255.255.255.0");
        nvs_get_string(nvs, "dns1", tmp, sizeof(tmp)); if (tmp[0]) copy_field(cfg_dns1, sizeof(cfg_dns1), tmp, "8.8.8.8");
        nvs_get_string(nvs, "dns2", tmp, sizeof(tmp)); if (tmp[0]) copy_field(cfg_dns2, sizeof(cfg_dns2), tmp, "8.8.4.4");
        uint8_t st = cfg_static ? 1 : 0;
        nvs_get_u8(nvs, "static", &st);
        cfg_static = st ? true : false;
        nvs_close(nvs);
    }
}

bool jr_wifi_is_configured(void) {
    if (cfg_ssid[0] == '\0') wifi_load_config();
    return strlen(cfg_ssid) > 0;
}
bool jr_wifi_is_connected(void) { return wifi_connected; }
const char *jr_wifi_ip(void) { return wifi_ip; }
const char *jr_wifi_ssid(void) { return cfg_ssid; }

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) esp_wifi_connect();
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) { wifi_connected=false; snprintf(wifi_ip,sizeof(wifi_ip),"sem wifi"); esp_wifi_connect(); }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        snprintf(wifi_ip, sizeof(wifi_ip), IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
        printf("JR_WIFI conectado ip=%s ssid=%s\n", wifi_ip, cfg_ssid);
    }
}

void jr_wifi_start(void) {
    if (wifi_started) return;
    wifi_load_config();
    if (!jr_wifi_is_configured()) { printf("JR_WIFI nao configurado. Modo Serial liberado. Configure pelo painel Serial.\n"); return; }
    ESP_ERROR_CHECK(ensure_nvs());
    ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t loop_result = esp_event_loop_create_default();
    if (loop_result != ESP_OK && loop_result != ESP_ERR_INVALID_STATE) ESP_ERROR_CHECK(loop_result);
    esp_netif_t *sta = esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(sta, cfg_host);
    if (cfg_static) {
        esp_netif_ip_info_t ip_info = {0};
        ip_info.ip.addr = ipaddr_addr(cfg_ip);
        ip_info.gw.addr = ipaddr_addr(cfg_gateway);
        ip_info.netmask.addr = ipaddr_addr(cfg_subnet);
        if (ip_info.ip.addr == IPADDR_NONE || ip_info.gw.addr == IPADDR_NONE || ip_info.netmask.addr == IPADDR_NONE) {
            printf("JR_WIFI IP fixo invalido. Confira configuracao no painel Serial.\n");
        } else {
            ESP_ERROR_CHECK(esp_netif_dhcpc_stop(sta));
            ESP_ERROR_CHECK(esp_netif_set_ip_info(sta, &ip_info));
            esp_netif_dns_info_t dns = {0};
            dns.ip.u_addr.ip4.addr = ipaddr_addr(cfg_dns1);
            dns.ip.type = ESP_IPADDR_TYPE_V4;
            esp_netif_set_dns_info(sta, ESP_NETIF_DNS_MAIN, &dns);
            dns.ip.u_addr.ip4.addr = ipaddr_addr(cfg_dns2);
            esp_netif_set_dns_info(sta, ESP_NETIF_DNS_BACKUP, &dns);
            printf("JR_WIFI usando IP fixo ip=%s gateway=%s mascara=%s\n", cfg_ip, cfg_gateway, cfg_subnet);
        }
    }
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));
    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, cfg_ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, cfg_pass, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    wifi_started = true;
    printf("JR_WIFI conectando ssid=%s\n", cfg_ssid);
}

bool jr_wifi_configure(const char *ssid, const char *password, const char *hostname, bool use_static_ip,
                       const char *static_ip, const char *gateway, const char *subnet,
                       const char *dns1, const char *dns2, char *response, unsigned response_len) {
    if (!ssid || !ssid[0]) {
        snprintf(response, response_len, "JR_WIFI_ERROR ssid_vazio");
        return false;
    }
    nvs_handle_t nvs;
    if (ensure_nvs() != ESP_OK || nvs_open("jr_wifi", NVS_READWRITE, &nvs) != ESP_OK) {
        snprintf(response, response_len, "JR_WIFI_ERROR nvs_open");
        return false;
    }
    nvs_set_str(nvs, "ssid", ssid);
    nvs_set_str(nvs, "pass", password ? password : "");
    nvs_set_str(nvs, "host", (hostname && hostname[0]) ? hostname : "jrbot");
    nvs_set_u8(nvs, "static", use_static_ip ? 1 : 0);
    nvs_set_str(nvs, "ip", (static_ip && static_ip[0]) ? static_ip : "192.168.0.50");
    nvs_set_str(nvs, "gw", (gateway && gateway[0]) ? gateway : "192.168.0.1");
    nvs_set_str(nvs, "mask", (subnet && subnet[0]) ? subnet : "255.255.255.0");
    nvs_set_str(nvs, "dns1", (dns1 && dns1[0]) ? dns1 : "8.8.8.8");
    nvs_set_str(nvs, "dns2", (dns2 && dns2[0]) ? dns2 : "8.8.4.4");
    esp_err_t err = nvs_commit(nvs);
    nvs_close(nvs);
    if (err != ESP_OK) {
        snprintf(response, response_len, "JR_WIFI_ERROR nvs_commit");
        return false;
    }
    wifi_load_config();
    if (!wifi_started) jr_wifi_start();
    else {
        wifi_config_t wifi_config = {0};
        strncpy((char*)wifi_config.sta.ssid, cfg_ssid, sizeof(wifi_config.sta.ssid) - 1);
        strncpy((char*)wifi_config.sta.password, cfg_pass, sizeof(wifi_config.sta.password) - 1);
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        esp_wifi_disconnect();
        esp_wifi_connect();
    }
    snprintf(response, response_len, "JR_WIFI_SALVO ssid=%s static=%d ip=%s aguarde_JR_WIFI_conectado", cfg_ssid, cfg_static ? 1 : 0, cfg_ip);
    printf("%s\n", response);
    return true;
}

bool jr_wifi_clear(char *response, unsigned response_len) {
    nvs_handle_t nvs;
    if (ensure_nvs() != ESP_OK || nvs_open("jr_wifi", NVS_READWRITE, &nvs) != ESP_OK) {
        snprintf(response, response_len, "JR_WIFI_ERROR nvs_open");
        return false;
    }
    nvs_erase_all(nvs);
    esp_err_t err = nvs_commit(nvs);
    nvs_close(nvs);
    cfg_ssid[0] = '\0';
    snprintf(response, response_len, err == ESP_OK ? "JR_WIFI_LIMPO reinicie_o_ESP32" : "JR_WIFI_ERROR nvs_commit");
    printf("%s\n", response);
    return err == ESP_OK;
}
