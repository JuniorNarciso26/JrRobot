#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

#include "jr_brain.h"
#include "jr_playground.h"
#include "jr_voice.h"

#define JR_PLAYGROUND_INTERVAL_MIN_MS 250U
#define JR_PLAYGROUND_INTERVAL_MAX_MS 60000U

static portMUX_TYPE playground_lock = portMUX_INITIALIZER_UNLOCKED;
static jr_playground_status_t playground_status = {
    .state = JR_PLAYGROUND_IDLE,
    .phrase = "",
    .interval_ms = 0,
    .detections = 0,
    .samples = 0,
    .skipped_interval = 0,
    .min_probability = 0.0f,
    .max_probability = 0.0f,
    .avg_probability = 0.0f,
    .last_probability = 0.0f,
    .last_recognized = "",
    .model = "",
    .started_ms = 0,
    .last_sample_ms = 0,
    .last_error = ESP_OK,
};
static double probability_sum = 0.0;

static uint64_t now_ms(void) {
    return (uint64_t)(esp_timer_get_time() / 1000);
}

static bool normalize_phrase(const char *input, char out[JR_PLAYGROUND_PHRASE_MAX]) {
    if (!input || !out) return false;
    size_t n = 0;
    bool pending_space = false;

    while (*input && isspace((unsigned char)*input)) input++;
    for (; *input; ++input) {
        unsigned char c = (unsigned char)*input;
        if (isspace(c)) {
            pending_space = n > 0;
            continue;
        }
        if (!isalnum(c)) return false;
        if (pending_space) {
            if (n + 1 >= JR_PLAYGROUND_PHRASE_MAX) return false;
            out[n++] = ' ';
            pending_space = false;
        }
        if (n + 1 >= JR_PLAYGROUND_PHRASE_MAX) return false;
        out[n++] = (char)toupper(c);
    }
    out[n] = '\0';
    return n > 0;
}

static void playground_detection(const char *recognized, const char *model_name, float probability) {
    uint64_t current_ms = now_ms();

    portENTER_CRITICAL(&playground_lock);
    if (playground_status.state != JR_PLAYGROUND_CALIBRATING) {
        portEXIT_CRITICAL(&playground_lock);
        return;
    }

    playground_status.detections++;
    if (playground_status.last_sample_ms != 0 &&
        current_ms - playground_status.last_sample_ms < playground_status.interval_ms) {
        playground_status.skipped_interval++;
        portEXIT_CRITICAL(&playground_lock);
        return;
    }

    playground_status.samples++;
    playground_status.last_sample_ms = current_ms;
    playground_status.last_probability = probability;
    if (playground_status.samples == 1 || probability < playground_status.min_probability)
        playground_status.min_probability = probability;
    if (playground_status.samples == 1 || probability > playground_status.max_probability)
        playground_status.max_probability = probability;
    probability_sum += probability;
    playground_status.avg_probability = (float)(probability_sum / (double)playground_status.samples);
    snprintf(playground_status.last_recognized, sizeof(playground_status.last_recognized), "%s",
             recognized && recognized[0] ? recognized : "JR BOT");
    snprintf(playground_status.model, sizeof(playground_status.model), "%s",
             model_name && model_name[0] ? model_name : "unknown");
    playground_status.last_error = ESP_OK;
    portEXIT_CRITICAL(&playground_lock);
}

const char *jr_playground_state_name(jr_playground_state_t state) {
    switch (state) {
        case JR_PLAYGROUND_IDLE: return "idle";
        case JR_PLAYGROUND_STARTING: return "starting";
        case JR_PLAYGROUND_CALIBRATING: return "calibrating";
        case JR_PLAYGROUND_STOPPING: return "stopping";
        case JR_PLAYGROUND_REVIEW: return "review";
        case JR_PLAYGROUND_ERROR: return "error";
        default: return "unknown";
    }
}

bool jr_playground_active(void) {
    portENTER_CRITICAL(&playground_lock);
    bool active = playground_status.state == JR_PLAYGROUND_STARTING ||
                  playground_status.state == JR_PLAYGROUND_CALIBRATING ||
                  playground_status.state == JR_PLAYGROUND_STOPPING;
    portEXIT_CRITICAL(&playground_lock);
    return active;
}

esp_err_t jr_playground_start(const char *phrase, uint32_t interval_ms) {
    char normalized[JR_PLAYGROUND_PHRASE_MAX] = {0};
    if (!normalize_phrase(phrase, normalized)) return ESP_ERR_INVALID_ARG;

    /* Primeira candidata: calibra somente o reconhecedor JrBot ja existente. */
    if (strcmp(normalized, "JR BOT") != 0) return ESP_ERR_NOT_SUPPORTED;
    if (interval_ms < JR_PLAYGROUND_INTERVAL_MIN_MS || interval_ms > JR_PLAYGROUND_INTERVAL_MAX_MS)
        return ESP_ERR_INVALID_ARG;
    if (jr_brain_enabled()) return ESP_ERR_INVALID_STATE;

    jr_voice_status_t voice = {0};
    jr_voice_get_status(&voice);
    if (voice.running) return ESP_ERR_INVALID_STATE;

    portENTER_CRITICAL(&playground_lock);
    if (playground_status.state == JR_PLAYGROUND_STARTING ||
        playground_status.state == JR_PLAYGROUND_CALIBRATING ||
        playground_status.state == JR_PLAYGROUND_STOPPING) {
        portEXIT_CRITICAL(&playground_lock);
        return ESP_ERR_INVALID_STATE;
    }
    playground_status = (jr_playground_status_t){0};
    playground_status.state = JR_PLAYGROUND_STARTING;
    playground_status.interval_ms = interval_ms;
    playground_status.last_error = ESP_OK;
    snprintf(playground_status.phrase, sizeof(playground_status.phrase), "%s", normalized);
    probability_sum = 0.0;
    portEXIT_CRITICAL(&playground_lock);

    esp_err_t err = jr_voice_start_calibration(playground_detection);
    if (err != ESP_OK) {
        portENTER_CRITICAL(&playground_lock);
        playground_status.state = JR_PLAYGROUND_ERROR;
        playground_status.last_error = err;
        portEXIT_CRITICAL(&playground_lock);
        return err;
    }

    jr_voice_get_status(&voice);
    portENTER_CRITICAL(&playground_lock);
    playground_status.state = JR_PLAYGROUND_CALIBRATING;
    playground_status.started_ms = now_ms();
    playground_status.last_error = ESP_OK;
    snprintf(playground_status.model, sizeof(playground_status.model), "%s",
             voice.model[0] ? voice.model : "unknown");
    portEXIT_CRITICAL(&playground_lock);
    return ESP_OK;
}

esp_err_t jr_playground_stop(void) {
    portENTER_CRITICAL(&playground_lock);
    bool can_stop = playground_status.state == JR_PLAYGROUND_STARTING ||
                    playground_status.state == JR_PLAYGROUND_CALIBRATING;
    if (!can_stop) {
        portEXIT_CRITICAL(&playground_lock);
        return ESP_ERR_INVALID_STATE;
    }
    playground_status.state = JR_PLAYGROUND_STOPPING;
    portEXIT_CRITICAL(&playground_lock);

    jr_voice_stop();
    jr_voice_status_t voice = {0};
    jr_voice_get_status(&voice);

    portENTER_CRITICAL(&playground_lock);
    if (voice.running) {
        playground_status.state = JR_PLAYGROUND_ERROR;
        playground_status.last_error = ESP_ERR_TIMEOUT;
        portEXIT_CRITICAL(&playground_lock);
        return ESP_ERR_TIMEOUT;
    }
    playground_status.state = JR_PLAYGROUND_REVIEW;
    playground_status.last_error = ESP_OK;
    portEXIT_CRITICAL(&playground_lock);
    return ESP_OK;
}

void jr_playground_get_status(jr_playground_status_t *out) {
    if (!out) return;
    portENTER_CRITICAL(&playground_lock);
    *out = playground_status;
    portEXIT_CRITICAL(&playground_lock);
}
