#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

typedef struct {
    bool configured;
    bool connected;
    bool network_ready;
    bool pending_restart;
    char ip[16];
    esp_err_t last_error;
    uint16_t last_disconnect_reason;
    uint32_t reconnect_attempts;
    bool ap_info_valid;
    int8_t rssi;
    uint8_t channel;
    uint8_t authmode;
    char bssid[18];
} jr_wifi_status_t;

typedef struct {
    bool valid;
    bool use_static_ip;
    bool password_set;
    uint8_t password_length;
    char source[16];
    char ssid[33];
    char host[33];
    char ip[16];
    char gateway[16];
    char mask[16];
    char dns1[16];
    char dns2[16];
} jr_wifi_config_info_t;

void jr_wifi_prepare(void);
void jr_wifi_start(void);
bool jr_wifi_network_ready(void);
void jr_wifi_get_status(jr_wifi_status_t *out);
void jr_wifi_get_active_config(jr_wifi_config_info_t *out);
bool jr_wifi_get_saved_config(jr_wifi_config_info_t *out);
bool jr_wifi_is_configured(void);
bool jr_wifi_is_connected(void);
const char *jr_wifi_ip(void);
const char *jr_wifi_ssid(void);
const char *jr_wifi_disconnect_reason_name(uint16_t reason);
bool jr_wifi_configure(const char *ssid, const char *password, const char *hostname, bool use_static_ip,
                       const char *static_ip, const char *gateway, const char *subnet,
                       const char *dns1, const char *dns2, char *response, unsigned response_len);
bool jr_wifi_clear(char *response, unsigned response_len);
