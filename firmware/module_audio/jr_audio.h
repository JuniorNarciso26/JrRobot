#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

typedef struct {
    bool running;
    uint32_t tests;
    uint32_t last_bytes;
    int last_frequency_hz;
    int last_duration_ms;
    esp_err_t last_error;
} jr_audio_diag_t;

esp_err_t jr_audio_bus_init(void);
bool jr_audio_bus_acquire(uint32_t timeout_ms);
void jr_audio_bus_release(void);

esp_err_t jr_audio_start(void);
void jr_audio_stop(void);
bool jr_audio_ready(void);
int jr_audio_volume(void);
void jr_audio_set_volume(int volume);
esp_err_t jr_audio_test_tone(int frequency_hz, int duration_ms);
esp_err_t jr_audio_play_pcm16_mono(const int16_t *samples, size_t sample_count, int sample_rate_hz);
void jr_audio_get_diag(jr_audio_diag_t *out);
const char *jr_audio_status_text(void);
