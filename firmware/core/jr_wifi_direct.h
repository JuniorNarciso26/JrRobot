#pragma once
#include <stdbool.h>
#include "esp_err.h"

/* Local SoftAP connection, not the Wi-Fi Alliance P2P protocol. NVS is
 * initialized by jr_wifi_prepare(). Changes apply only after reboot. */
bool jr_wifi_direct_enabled(void);
bool jr_wifi_direct_active(void);
esp_err_t jr_wifi_direct_configure(bool enabled, const char *password);
esp_err_t jr_wifi_direct_start(void);
void jr_wifi_direct_stop(void);
const char *jr_wifi_direct_ssid(void);
#define JR_WIFI_DIRECT_IP "192.168.4.1"
