#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

typedef void (*jr_voice_wake_cb_t)(const char *keyword, const char *model_name, int wake_index);

typedef struct {
    bool running;
    bool mic_ready;
    uint32_t frames;
    uint32_t detections;
    uint32_t last_level;
    int sample_rate;
    int chunk_samples;
    esp_err_t last_error;
    char engine[32];
    char model[64];
    char wakeword[40];
} jr_voice_status_t;

esp_err_t jr_voice_start(jr_voice_wake_cb_t wake_cb);
void jr_voice_stop(void);
void jr_voice_get_status(jr_voice_status_t *out);
