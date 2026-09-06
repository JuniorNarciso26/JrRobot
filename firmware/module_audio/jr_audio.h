#pragma once

#include <stdbool.h>
#include "esp_err.h"

esp_err_t jr_audio_start(void);
void jr_audio_stop(void);
bool jr_audio_ready(void);
int jr_audio_volume(void);
void jr_audio_set_volume(int volume);
esp_err_t jr_audio_test_tone(int frequency_hz, int duration_ms);
const char *jr_audio_status_text(void);
