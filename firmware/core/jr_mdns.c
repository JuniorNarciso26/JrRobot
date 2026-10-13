/* JrBot experiment: stable local name with DHCP-managed addresses. */
#include "jr_mdns.h"

#include "esp_check.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "mdns.h"
#include "esp_heap_caps.h"
#include "sdkconfig.h"

static const char *TAG = "JR_MDNS";
static bool s_started;


static void mdns_memory_report(const char *stage)
{
#if CONFIG_MDNS_MEMORY_ALLOC_SPIRAM
    const char *storage="PSRAM";
#else
    const char *storage="INTERNAL";
#endif
    ESP_LOGI(TAG, "MDNS_MEMORY stage=%s allocations=%s internal_free=%u internal_largest=%u dma_free=%u dma_largest=%u psram_free=%u",
             stage, storage,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

esp_err_t jr_mdns_start(void)
{
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    ESP_RETURN_ON_FALSE(sta, ESP_ERR_INVALID_STATE, TAG, "Wi-Fi station interface not ready");
    if (s_started) {
        /* Re-enable and announce the interface after every DHCP lease. */
        ESP_RETURN_ON_ERROR(mdns_netif_action(sta, MDNS_EVENT_ENABLE_IP4),
                            TAG, "mDNS IPv4 enable failed");
        ESP_RETURN_ON_ERROR(mdns_netif_action(sta, MDNS_EVENT_ANNOUNCE_IP4),
                            TAG, "mDNS IPv4 announcement failed");
        ESP_LOGI(TAG, "DHCP address announced: http://jrbot.local");
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(esp_netif_set_hostname(sta, "jrbot"), TAG, "Set DHCP hostname failed");
    ESP_LOGI(TAG, "MEMORY_CONFIG mdns_heap=%s mdns_stack_bytes=%u event_stack_bytes=%u sockets=%u",
#if CONFIG_MDNS_MEMORY_ALLOC_SPIRAM
             "PSRAM",
#else
             "INTERNAL",
#endif
             (unsigned)CONFIG_MDNS_TASK_STACK_SIZE,
             (unsigned)CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE,
             (unsigned)CONFIG_LWIP_MAX_SOCKETS);
    mdns_memory_report("before_init");
    ESP_RETURN_ON_ERROR(mdns_init(), TAG, "mDNS init failed");
    esp_err_t ret = mdns_hostname_set("jrbot");
    if (ret == ESP_OK) {
        ret = mdns_instance_name_set("JrBot");
    }
    if (ret == ESP_OK) {
        ret = mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
    }
    if (ret != ESP_OK) {
        mdns_free();
        return ret;
    }
    /* mDNS follows esp_netif Wi-Fi/IP events. Do not delegate or persist a
     * literal IP: reconnects and DHCP changes must publish the current address.
     */
    s_started = true;
    mdns_memory_report("after_init");
    esp_netif_ip_info_t info = {0};
    if (esp_netif_get_ip_info(sta, &info) == ESP_OK && info.ip.addr != 0) {
        return jr_mdns_start();
    }
    ESP_LOGI(TAG, "Local name ready: jrbot.local (automatic DHCP address)");
    return ESP_OK;
}
