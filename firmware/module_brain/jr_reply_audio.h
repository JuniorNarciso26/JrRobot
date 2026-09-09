#pragma once

#include <stdbool.h>
#include "esp_err.h"

// Resposta local curta gerada no proprio ESP32, sem nuvem/TTS externo.
void jr_reply_prepare_oi_async(void);
bool jr_reply_oi_ready(void);
esp_err_t jr_reply_play_oi(void);
