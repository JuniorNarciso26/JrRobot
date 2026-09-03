#pragma once
#include <stdbool.h>

void jr_wifi_start(void);
bool jr_wifi_is_configured(void);
bool jr_wifi_is_connected(void);
const char *jr_wifi_ip(void);
const char *jr_wifi_ssid(void);
bool jr_wifi_configure(const char *ssid, const char *password, const char *hostname, bool use_static_ip,
                       const char *static_ip, const char *gateway, const char *subnet,
                       const char *dns1, const char *dns2, char *response, unsigned response_len);
bool jr_wifi_clear(char *response, unsigned response_len);
