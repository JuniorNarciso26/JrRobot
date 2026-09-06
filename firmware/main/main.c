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
    ESP_LOGI("jrbot_main", "BOOT %s profile=%s OLED=%s SDA=1 SCL=2",
             JR_APP_VERSION, JR_PROFILE_NAME, JR_OLED_ENABLED ? "enabled" : "disabled");
    jr_wifi_prepare();
    if (jr_commands_init() != ESP_OK)
        ESP_LOGE("jrbot_main", "Command mutex unavailable");
    jr_terminal_start();
    /* No amplifier, camera, microphone or servo is started at boot. */
    if (JR_OLED_ENABLED) {
        if (jr_face_start() != ESP_OK)
            ESP_LOGW("jrbot_main", "OLED offline; automatic retry enabled; terminal remains available");
    } else {
        ESP_LOGW("jrbot_main", "OLED disabled by configuration; no OLED I2C access");
    }
    jr_wifi_start();
    if (jr_wifi_network_ready()) jr_portal_start();
    ESP_LOGI("jrbot_main", "Diagnostic ready: version, status, audio_test, camera_test, mic_test");
    if (JR_OLED_ENABLED) jr_face_loop();
    /* Keep yielding: no display, render loop or retry is required for commands. */
    for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
}
