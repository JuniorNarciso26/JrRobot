#include <stdbool.h>
#include <stdio.h>
#include "esp_heap_caps.h"
#include "esp_netif.h"
#include "mdns.h"
#include "sdkconfig.h"
#include "jr_mdns.h"
#include "jr_mode_manager.h"
#include "jr_wifi.h"

#if !CONFIG_MDNS_MEMORY_ALLOC_SPIRAM
#error "JrBot mDNS requires CONFIG_MDNS_MEMORY_ALLOC_SPIRAM=y; use INSTALAR.bat"
#endif
#if !CONFIG_MDNS_TASK_CREATE_FROM_INTERNAL
#error "Keep the mDNS task stack in internal RAM"
#endif

static bool attempted;

static void memory_report(const char *stage) {
    printf("JR_MDNS_MEMORY stage=%s internal_free=%u internal_largest=%u dma_free=%u dma_largest=%u psram_free=%u\n",
        stage,(unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

void jr_mdns_poll(void) {
    if (attempted || !jr_wifi_is_connected() || jr_mode_get()!=JR_MODE_IDLE) return;
    attempted=true;
    /* This runs on app_main's stack, never in the system-event callback. */
    esp_netif_t *sta=esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    const char *hostname=NULL;
    esp_err_t err=sta?esp_netif_get_hostname(sta,&hostname):ESP_ERR_INVALID_STATE;
    if (err!=ESP_OK || !hostname || !*hostname) {
        printf("JR_MDNS_ERROR stage=hostname result=%s acesso_por_ip_disponivel=1\n",
            esp_err_to_name(err==ESP_OK?ESP_ERR_INVALID_STATE:err));
        return;
    }
    printf("JR_MDNS_CONFIG heap=PSRAM stack=INTERNAL stack_bytes=%u interfaces=%u services=%u event_stack_bytes=%u sockets=%u\n",
        (unsigned)CONFIG_MDNS_TASK_STACK_SIZE,(unsigned)CONFIG_MDNS_MAX_INTERFACES,
        (unsigned)CONFIG_MDNS_MAX_SERVICES,(unsigned)CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE,
        (unsigned)CONFIG_LWIP_MAX_SOCKETS);
    memory_report("before_init");
    const char *stage="init";
    bool initialized=false;
    err=mdns_init();
    if (err!=ESP_OK) goto fail;
    initialized=true;
    stage="hostname"; err=mdns_hostname_set(hostname); if (err!=ESP_OK) goto fail;
    stage="service"; err=mdns_service_add("JrBot","_https","_tcp",443,NULL,0); if (err!=ESP_OK) goto fail;
    /* We start after GOT_IP; explicitly enable/announce the already-connected STA.
     * The component's own event handlers follow future disconnects and IP changes. */
    stage="enable"; err=mdns_netif_action(sta,MDNS_EVENT_ENABLE_IP4); if (err!=ESP_OK) goto fail;
    stage="announce"; err=mdns_netif_action(sta,MDNS_EVENT_ANNOUNCE_IP4); if (err!=ESP_OK) goto fail;
    memory_report("after_init");
    jr_wifi_status_t wifi; jr_wifi_get_status(&wifi);
    printf("JR_MDNS_READY url=https://%s.local ip=%s interface=STA\n",hostname,wifi.ip);
    return;
fail:
    if (initialized) mdns_free();
    memory_report("after_failure");
    printf("JR_MDNS_ERROR stage=%s result=%s acesso_por_ip_disponivel=1\n",stage,esp_err_to_name(err));
}
