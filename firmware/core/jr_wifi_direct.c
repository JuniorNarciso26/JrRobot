#include <stdio.h>
#include <string.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "nvs.h"
#include "jr_wifi_direct.h"

static esp_netif_t *direct_ap;
static bool driver_initialized, running;
static char direct_ssid[33];

bool jr_wifi_direct_enabled(void) {
    nvs_handle_t h;
    uint8_t enabled = 0;
    if (nvs_open("jr_direct", NVS_READONLY, &h) == ESP_OK) {
        (void)nvs_get_u8(h, "enabled", &enabled);
        nvs_close(h);
    }
    return enabled == 1;
}

bool jr_wifi_direct_active(void) { return running; }
const char *jr_wifi_direct_ssid(void) { return direct_ssid; }

esp_err_t jr_wifi_direct_configure(bool enabled, const char *password) {
    /* Require a new user-selected WPA2 password whenever enabling. Never
     * expose it through status or logs; saved router profiles are untouched. */
    size_t length = password ? strlen(password) : 0;
    if (enabled && (length < 8 || length > 63)) return ESP_ERR_INVALID_ARG;
    for (size_t i = 0; enabled && i < length; i++)
        if ((unsigned char)password[i] < 32 || (unsigned char)password[i] > 126)
            return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;
    esp_err_t err = nvs_open("jr_direct", NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    if (enabled) err = nvs_set_str(h, "password", password);
    if (err == ESP_OK) err = nvs_set_u8(h, "enabled", enabled ? 1 : 0);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

void jr_wifi_direct_stop(void) {
    if (driver_initialized) {
        if (running) (void)esp_wifi_stop();
        (void)esp_wifi_deinit();
        driver_initialized = false;
    }
    if (direct_ap) {
        esp_netif_destroy_default_wifi(direct_ap);
        direct_ap = NULL;
    }
    running = false;
}

esp_err_t jr_wifi_direct_start(void) {
    if (running) return ESP_OK;
    if (!jr_wifi_direct_enabled()) return ESP_ERR_INVALID_STATE;
    wifi_config_t config = {0};
    nvs_handle_t h;
    esp_err_t err = nvs_open("jr_direct", NVS_READONLY, &h);
    if (err != ESP_OK) return err;
    size_t length = sizeof(config.ap.password);
    err = nvs_get_str(h, "password", (char *)config.ap.password, &length);
    nvs_close(h);
    if (err != ESP_OK || length < 9 || length > 64) {
        memset(&config, 0, sizeof(config));
        return ESP_ERR_INVALID_STATE;
    }
    err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) goto fail;
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) goto fail;
    esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_WIFI_AP();
    direct_ap = esp_netif_new(&netif_config);
    if (!direct_ap) { err = ESP_ERR_NO_MEM; goto fail; }
    err = esp_netif_attach_wifi_ap(direct_ap);
    if (err != ESP_OK) goto fail;
    err = esp_wifi_set_default_wifi_ap_handlers();
    if (err != ESP_OK) goto fail;
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&init);
    if (err != ESP_OK) goto fail;
    driver_initialized = true;
    err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (err != ESP_OK) goto fail;
    uint8_t mac[6];
    err = esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    if (err != ESP_OK) goto fail;
    snprintf(direct_ssid, sizeof(direct_ssid), "JrBot-%02X%02X%02X", mac[3], mac[4], mac[5]);
    memcpy(config.ap.ssid, direct_ssid, strlen(direct_ssid));
    config.ap.ssid_len = strlen(direct_ssid);
    config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    config.ap.max_connection = 2;
    config.ap.channel = 1;
    config.ap.pmf_cfg.capable = true;
    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) goto fail;
    err = esp_wifi_set_config(WIFI_IF_AP, &config);
    if (err != ESP_OK) goto fail;
    err = esp_wifi_start();
    if (err != ESP_OK) goto fail;
    running = true;
    memset(&config, 0, sizeof(config));
    ESP_LOGI("jrbot_direct", "JR_WIFI_DIRECT_READY ssid=%s security=WPA2 url=https://" JR_WIFI_DIRECT_IP, direct_ssid);
    return ESP_OK;
fail:
    memset(&config, 0, sizeof(config));
    jr_wifi_direct_stop();
    ESP_LOGE("jrbot_direct", "JR_WIFI_DIRECT_ERROR result=%s", esp_err_to_name(err));
    return err;
}
