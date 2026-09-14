#pragma once
#include <stdbool.h>
#include "esp_err.h"
typedef struct {
    bool configured, connected, network_ready, pending_restart;
    char ip[16];
    esp_err_t last_error;
    unsigned disconnect_reason;
} jr_wifi_status_t;
void jr_wifi_prepare(void);
void jr_wifi_start(void);
bool jr_wifi_network_ready(void);
void jr_wifi_get_status(jr_wifi_status_t *out);
bool jr_wifi_is_configured(void);
bool jr_wifi_is_connected(void);
const char *jr_wifi_ip(void);
const char *jr_wifi_ssid(void);
/* Retorna dados salvos sem expor a senha; SSID e hostname sao percent-encoded. */
bool jr_wifi_format_info(char *response, unsigned response_len);
bool jr_wifi_configure(const char *ssid, const char *password, const char *hostname, bool use_static_ip,
                       const char *static_ip, const char *gateway, const char *subnet,
                       const char *dns1, const char *dns2, char *response, unsigned response_len);
bool jr_wifi_clear(char *response, unsigned response_len);
