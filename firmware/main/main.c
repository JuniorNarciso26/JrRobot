#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "jr_commands.h"
#include "jr_audio.h"
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

    if (jr_audio_start() != ESP_OK) {
        ESP_LOGW(TAG, "Modulo audio falhou. Portal, camera e OLED continuam para diagnostico.");
    }

    ESP_LOGW(TAG, "Teste de audio ativo: camera nao inicia automaticamente porque usa GPIO15/16/17 neste hardware de teste.");

    jr_wifi_start();
    jr_portal_start();
    jr_terminal_start();
    jr_face_loop();
}
