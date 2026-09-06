#include "esp_log.h"
#include "jr_board.h"
#include "jr_commands.h"
#include "jr_audio.h"
#include "jr_face.h"
#include "jr_portal.h"
#include "jr_wifi.h"

void app_main(void) {
    ESP_LOGI("jrbot_main","JrBot V2 profile=%s OLED SDA=1 SCL=2",JR_PROFILE_NAME);
    jr_wifi_prepare();
    if (jr_commands_init()!=ESP_OK) ESP_LOGE("jrbot_main","Command mutex unavailable");
    if (JR_AUDIO_ENABLED && jr_audio_start()!=ESP_OK) ESP_LOGW("jrbot_main","Audio offline; diagnostic remains available");
    jr_terminal_start();
    if (jr_face_start()!=ESP_OK) ESP_LOGW("jrbot_main","OLED offline; automatic retry enabled");
    jr_wifi_start();
    if (jr_wifi_network_ready()) jr_portal_start();
    ESP_LOGI("jrbot_main","Camera and servo disabled pending physical validation");
    jr_face_loop();
}
