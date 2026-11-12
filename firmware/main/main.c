#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "jr_config.h"
#include "jr_board.h"
#include "jr_commands.h"
#include "jr_usb_terminal.h"
#include "jr_face.h"
#include "jr_portal.h"
#include "jr_wifi.h"
#include "jr_wifi_provisioning.h"
#include "jr_mdns.h"

#define JR_OLED_RETRY_MS 30000U
#define JR_WIFI_SETUP_TIMEOUT_MS 30000U

static uint32_t now_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

void app_main(void) {
    ESP_LOGI("jrbot_main", "BOOT %s build_sp=%s hardware=%s profile=%s",
             JR_APP_VERSION,JR_BUILD_STAMP_SP,JR_PINMAP_REVISION,JR_PROFILE_NAME);
    ESP_LOGI("jrbot_main", "OLED SSD1306 enabled SDA=%d SCL=%d",JR_OLED_SDA_GPIO,JR_OLED_SCL_GPIO);
    ESP_LOGI("jrbot_main", "MAX98357A enabled BCLK=%d WS=%d DIN=%d",JR_AUDIO_BCLK_GPIO,JR_AUDIO_LRC_GPIO,JR_AUDIO_DIN_GPIO);
    ESP_LOGI("jrbot_main", "MS3625 enabled SCK=%d WS=%d SD=%d",JR_MIC_SCK_GPIO,JR_MIC_WS_GPIO,JR_MIC_SD_GPIO);
    ESP_LOGI("jrbot_main", "OV5640 enabled on board camera connector");

    jr_wifi_prepare();
    if (jr_commands_init() != ESP_OK) ESP_LOGE("jrbot_main", "Command mutex unavailable");
    jr_terminal_start();
    jr_usb_terminal_start();

    uint32_t last_oled_probe = now_ms();
    if (jr_face_start() != ESP_OK)
        ESP_LOGW("jrbot_main", "OLED offline; retry limitado a cada 30 s para preservar painel e testes");

    jr_wifi_provisioning_button_init();
    bool setup_mode = jr_wifi_provisioning_boot_requested();
    if (setup_mode && jr_wifi_provisioning_start() != ESP_OK) setup_mode = false;
    if (!setup_mode) jr_wifi_start();
    bool portal_started = false;
    uint32_t wifi_start_ms = now_ms();
    ESP_LOGI("jrbot_main", "JrBot ready: camera/mic/status e audio sob demanda; OLED isolado quando offline");

    for (;;) {
        uint32_t ms = now_ms();
        if (setup_mode) jr_wifi_provisioning_poll();
        if (!setup_mode) {
            jr_wifi_provisioning_button_poll(ms);
            if (!portal_started && jr_wifi_is_connected()) {
                jr_portal_start();
                portal_started = true;
            }
            if (!portal_started && !jr_wifi_is_connected() &&
                (uint32_t)(ms - wifi_start_ms) >= JR_WIFI_SETUP_TIMEOUT_MS) {
                if (jr_wifi_provisioning_start() == ESP_OK) setup_mode = true;
                else wifi_start_ms = ms;
            }
        }
        jr_mdns_poll();
        jr_face_status_t face;
        jr_face_get_status(&face);

        if (face.state == JR_OLED_READY) {
            jr_face_step(ms);
            vTaskDelay(pdMS_TO_TICKS(80));
            continue;
        }

        if ((uint32_t)(ms - last_oled_probe) >= JR_OLED_RETRY_MS) {
            last_oled_probe = ms;
            (void)jr_face_start();
        }
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
