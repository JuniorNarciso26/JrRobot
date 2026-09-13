#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define JR_VOICE_JRBOT_DEFAULT_MIN_PROBABILITY 0.60f
#define JR_VOICE_JRBOT_MIN_ALLOWED_PROBABILITY 0.30f
#define JR_VOICE_JRBOT_MAX_ALLOWED_PROBABILITY 0.90f

typedef void (*jr_voice_wake_cb_t)(const char *keyword, const char *model_name, int wake_index, float probability);
typedef void (*jr_voice_calibration_cb_t)(const char *recognized, const char *model_name, float probability);

typedef struct {
    bool running;
    bool mic_ready;
    uint32_t frames;
    uint32_t detections;
    uint32_t rejected_low_confidence;
    uint32_t last_level;
    float last_probability;
    float min_probability;
    int sample_rate;
    int chunk_samples;
    esp_err_t last_error;
    char engine[32];
    char model[64];
    char wakeword[40];
} jr_voice_status_t;

esp_err_t jr_voice_start(jr_voice_wake_cb_t wake_cb);
esp_err_t jr_voice_start_calibration(jr_voice_calibration_cb_t calibration_cb);
void jr_voice_stop(void);
void jr_voice_get_status(jr_voice_status_t *out);
bool jr_voice_calibration_active(void);
esp_err_t jr_voice_set_min_probability(float probability);
float jr_voice_get_min_probability(void);
