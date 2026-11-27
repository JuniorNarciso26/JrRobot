#pragma once
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2s_std.h"
esp_err_t jr_stt_reserve(void);
esp_err_t jr_stt_open(void);
void jr_stt_close(void);
esp_err_t jr_stt_capture(i2s_chan_handle_t rx, const volatile bool *cancel);
void jr_stt_transcribe(const volatile bool *cancel);
