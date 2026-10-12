/* JrBot experiment: stable local name with DHCP-managed addresses. */
#include "jr_mdns.h"

#include "esp_check.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "mdns.h"

static const char *TAG = "JR_MDNS";
static bool s_started;

esp_err_t jr_mdns_start(void)
{
    if (s_started) {
        return ESP_OK;
    }
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    ESP_RETURN_ON_FALSE(sta, ESP_ERR_INVALID_STATE, TAG, "Wi-Fi station interface not ready");
    ESP_RETURN_ON_ERROR(esp_netif_set_hostname(sta, "jrbot"), TAG, "Set DHCP hostname failed");
    ESP_RETURN_ON_ERROR(mdns_init(), TAG, "mDNS init failed");
    esp_err_t ret = mdns_hostname_set("jrbot");
    if (ret == ESP_OK) {
        ret = mdns_instance_name_set("JrBot");
    }
    if (ret != ESP_OK) {
        mdns_free();
        return ret;
    }
    /* mDNS follows esp_netif Wi-Fi/IP events. Do not delegate or persist a
     * literal IP: reconnects and DHCP changes must publish the current address.
     */
    s_started = true;
    ESP_LOGI(TAG, "Local name ready: jrbot.local (automatic DHCP address)");
    return ESP_OK;
}
