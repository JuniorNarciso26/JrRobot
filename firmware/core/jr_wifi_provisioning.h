#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/* SoftAP setup adapted for JrBot from test/esp-wifi-provisioning. */
bool jr_wifi_provisioning_boot_requested(void);
esp_err_t jr_wifi_provisioning_start(void);
void jr_wifi_provisioning_button_init(void);
void jr_wifi_provisioning_button_poll(uint32_t now_ms);
