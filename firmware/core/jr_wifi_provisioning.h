#pragma once
#include <stdbool.h>

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/* SoftAP setup adapted for JrBot from test/esp-wifi-provisioning. */
bool jr_wifi_provisioning_boot_requested(void);
esp_err_t jr_wifi_provisioning_start(void);
void jr_wifi_provisioning_poll(void);
/* Automatic startup recovery: one software restart before exposing setup AP. */
bool jr_wifi_provisioning_auto_restart_once(void);
void jr_wifi_provisioning_recovery_connected(void);
void jr_wifi_provisioning_button_init(void);
void jr_wifi_provisioning_button_poll(uint32_t now_ms);
