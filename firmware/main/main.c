#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "jr_commands.h"
#include "jr_camera.h"
#include "jr_face.h"
#include "jr_portal.h"
#include "jr_wifi.h"

static const char *TAG = "jrbot_main";

void app_main(void) {
    ESP_LOGI(TAG, "JrBot modular iniciando");

    if (jr_face_start() != ESP_OK) {
        ESP_LOGE(TAG, "Modulo face falhou. Parando para proteger o teste atual.");
        return;
    }

    if (jr_camera_start() != ESP_OK) {
        ESP_LOGW(TAG, "Modulo camera falhou. Portal e OLED continuam para diagnostico.");
    }

    jr_wifi_start();
    jr_portal_start();
    jr_terminal_start();
    jr_face_loop();
}
