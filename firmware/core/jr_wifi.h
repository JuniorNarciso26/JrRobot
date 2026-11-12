#pragma once
#include <stdbool.h>
#include "esp_err.h"
typedef struct { bool configured, connected, network_ready, pending_restart; char ip[16]; esp_err_t last_error; } jr_wifi_status_t;
void jr_wifi_prepare(void);
void jr_wifi_start(void);
void jr_wifi_stop(void);
bool jr_wifi_network_ready(void);
void jr_wifi_get_status(jr_wifi_status_t *out);
bool jr_wifi_is_configured(void);
bool jr_wifi_is_connected(void);
const char *jr_wifi_ip(void);
const char *jr_wifi_ssid(void);
bool jr_wifi_configure(const char *ssid, const char *password, const char *hostname, bool use_static_ip,
                       const char *static_ip, const char *gateway, const char *subnet,
                       const char *dns1, const char *dns2, char *response, unsigned response_len);
bool jr_wifi_clear(char *response, unsigned response_len);
/* On-demand scan on the existing connected station; blocked outside idle mode. */
bool jr_wifi_scan(char *response, unsigned response_len);
bool jr_wifi_profiles_list(char *response, unsigned response_len);
bool jr_wifi_profile_use(unsigned id, char *response, unsigned response_len);
bool jr_wifi_profile_delete(unsigned id, char *response, unsigned response_len);
bool jr_wifi_profile_save(const char *ssid, const char *password, const char *hostname, bool use_static_ip,
                         const char *static_ip, const char *gateway, const char *subnet,
                         const char *dns1, const char *dns2, char *response, unsigned response_len);
