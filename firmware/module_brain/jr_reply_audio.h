#pragma once

#include "esp_err.h"

// Resposta local curta gerada no proprio ESP32, sem nuvem/TTS externo.
esp_err_t jr_reply_play_oi(void);
