#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define JR_PLAYGROUND_PHRASE_MAX 40
#define JR_PLAYGROUND_RECOGNIZED_MAX 64
#define JR_PLAYGROUND_MODEL_MAX 64

typedef enum {
    JR_PLAYGROUND_IDLE = 0,
    JR_PLAYGROUND_STARTING,
    JR_PLAYGROUND_CALIBRATING,
    JR_PLAYGROUND_STOPPING,
    JR_PLAYGROUND_REVIEW,
    JR_PLAYGROUND_ERROR,
} jr_playground_state_t;

typedef struct {
    jr_playground_state_t state;
    char phrase[JR_PLAYGROUND_PHRASE_MAX];
    uint32_t interval_ms;
    uint32_t detections;
    uint32_t samples;
    uint32_t skipped_interval;
    float min_probability;
    float max_probability;
    float avg_probability;
    float last_probability;
    char last_recognized[JR_PLAYGROUND_RECOGNIZED_MAX];
    char model[JR_PLAYGROUND_MODEL_MAX];
    uint64_t started_ms;
    uint64_t last_sample_ms;
    esp_err_t last_error;
} jr_playground_status_t;

const char *jr_playground_state_name(jr_playground_state_t state);
esp_err_t jr_playground_start(const char *phrase, uint32_t interval_ms);
esp_err_t jr_playground_stop(void);
void jr_playground_get_status(jr_playground_status_t *out);
bool jr_playground_active(void);
