#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "lwip/inet.h"
#include "nvs_flash.h"

#include "jr_config.h"
#include "jr_wifi.h"

static bool wifi_connected = false;
static char wifi_ip[16] = "sem wifi";

bool jr_wifi_is_connected(void) { return wifi_connected; }
const char *jr_wifi_ip(void) { return wifi_ip; }
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) esp_wifi_connect();
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) { wifi_connected=false; snprintf(wifi_ip,sizeof(wifi_ip),"sem wifi"); esp_wifi_connect(); }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        snprintf(wifi_ip, sizeof(wifi_ip), IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
        printf("JR_WIFI conectado ip=%s\n", wifi_ip);
    }
}
void jr_wifi_start(void) {
    if (strlen(JR_WIFI_SSID) == 0) { printf("JR_WIFI nao configurado. Rode CONFIGURAR_WIFI.bat antes de instalar.\n"); return; }
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta = esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(sta, JR_WIFI_HOSTNAME);
    if (JR_WIFI_USE_STATIC_IP) {
        esp_netif_ip_info_t ip_info = {0};
        ip_info.ip.addr = ipaddr_addr(JR_WIFI_STATIC_IP);
        ip_info.gw.addr = ipaddr_addr(JR_WIFI_GATEWAY);
        ip_info.netmask.addr = ipaddr_addr(JR_WIFI_SUBNET);
        if (ip_info.ip.addr == IPADDR_NONE || ip_info.gw.addr == IPADDR_NONE || ip_info.netmask.addr == IPADDR_NONE) {
            printf("JR_WIFI IP fixo invalido. Confira credencial/wifi.txt\n");
        } else {
            ESP_ERROR_CHECK(esp_netif_dhcpc_stop(sta));
            ESP_ERROR_CHECK(esp_netif_set_ip_info(sta, &ip_info));
            esp_netif_dns_info_t dns = {0};
            dns.ip.u_addr.ip4.addr = ipaddr_addr(JR_WIFI_DNS1);
            dns.ip.type = ESP_IPADDR_TYPE_V4;
            esp_netif_set_dns_info(sta, ESP_NETIF_DNS_MAIN, &dns);
            dns.ip.u_addr.ip4.addr = ipaddr_addr(JR_WIFI_DNS2);
            esp_netif_set_dns_info(sta, ESP_NETIF_DNS_BACKUP, &dns);
            printf("JR_WIFI usando IP fixo ip=%s gateway=%s mascara=%s\n", JR_WIFI_STATIC_IP, JR_WIFI_GATEWAY, JR_WIFI_SUBNET);
        }
    }
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));
    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, JR_WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, JR_WIFI_PASSWORD, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    printf("JR_WIFI conectando ssid=%s\n", JR_WIFI_SSID);
}

