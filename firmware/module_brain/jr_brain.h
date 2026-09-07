#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

typedef enum {
    JR_BRAIN_OFF = 0,
    JR_BRAIN_STARTING,
    JR_BRAIN_LISTENING,
    JR_BRAIN_STOPPING,
    JR_BRAIN_ERROR,
} jr_brain_state_t;

typedef struct {
    bool enabled;
    bool listening;
    bool mic_ready;
    jr_brain_state_t state;
    uint32_t triggers;
    uint32_t frames;
    uint32_t last_level;
    float last_probability;
    esp_err_t last_error;
    char engine[40];
    char last_event[32];
} jr_brain_status_t;

esp_err_t jr_brain_set_enabled(bool enabled);
bool jr_brain_enabled(void);
void jr_brain_get_status(jr_brain_status_t *out);
const char *jr_brain_state_name(jr_brain_state_t state);

/* Bancada: valida a cadeia autonomo -> evento -> rosto sem depender do reconhecimento. */
esp_err_t jr_brain_trigger_test(void);
