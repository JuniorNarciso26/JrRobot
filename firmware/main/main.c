#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "jr_config.h"
#include "jr_board.h"
#include "jr_commands.h"
#include "jr_face.h"
#include "jr_portal.h"
#include "jr_wifi.h"

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
    if (jr_face_start() != ESP_OK)
        ESP_LOGW("jrbot_main", "OLED offline; automatic retry active; terminal remains available");
    jr_wifi_start();
    if (jr_wifi_network_ready()) jr_portal_start();
    ESP_LOGI("jrbot_main", "JrBot ready: Atualizar estado detects camera/mic; amplifier test is on demand");
    jr_face_loop();
    for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
}
