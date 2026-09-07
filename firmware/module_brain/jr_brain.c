#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "jr_brain.h"
#include "jr_face.h"

static const char *TAG = "jrbot_brain";
static portMUX_TYPE brain_lock = portMUX_INITIALIZER_UNLOCKED;
static bool brain_enabled = false;
static jr_brain_state_t brain_state = JR_BRAIN_OFF;
static uint32_t brain_triggers = 0;
static uint32_t brain_frames = 0;
static uint32_t brain_last_level = 0;
static float brain_last_probability = 0.0f;
static esp_err_t brain_last_error = ESP_OK;
static char brain_last_event[32] = "boot";

static void set_event(const char *event) {
    portENTER_CRITICAL(&brain_lock);
    snprintf(brain_last_event, sizeof(brain_last_event), "%s", event ? event : "unknown");
    portEXIT_CRITICAL(&brain_lock);
}

const char *jr_brain_state_name(jr_brain_state_t state) {
    switch (state) {
        case JR_BRAIN_OFF: return "off";
        case JR_BRAIN_STARTING: return "starting";
        case JR_BRAIN_LISTENING: return "listening";
        case JR_BRAIN_STOPPING: return "stopping";
        case JR_BRAIN_ERROR: return "error";
        default: return "unknown";
    }
}

esp_err_t jr_brain_set_enabled(bool enabled) {
    portENTER_CRITICAL(&brain_lock);
    if (brain_enabled == enabled) {
        portEXIT_CRITICAL(&brain_lock);
        return ESP_OK;
    }
    brain_state = enabled ? JR_BRAIN_STARTING : JR_BRAIN_STOPPING;
    brain_enabled = enabled;
    portEXIT_CRITICAL(&brain_lock);

    if (enabled) {
        set_event("autonomous_on");
        ESP_LOGI(TAG, "JR_BRAIN event=autonomous_on engine=bench_stub wakeword=JrBot");
        portENTER_CRITICAL(&brain_lock);
        brain_state = JR_BRAIN_LISTENING;
        brain_last_error = ESP_OK;
        portEXIT_CRITICAL(&brain_lock);
        ESP_LOGI(TAG, "JR_BRAIN event=listening wakeword=JrBot note=custom_wakenet_pending");
    } else {
        set_event("autonomous_off");
        ESP_LOGI(TAG, "JR_BRAIN event=autonomous_off");
        portENTER_CRITICAL(&brain_lock);
        brain_state = JR_BRAIN_OFF;
        brain_last_error = ESP_OK;
        portEXIT_CRITICAL(&brain_lock);
    }
    return ESP_OK;
}

bool jr_brain_enabled(void) {
    portENTER_CRITICAL(&brain_lock);
    bool enabled = brain_enabled;
    portEXIT_CRITICAL(&brain_lock);
    return enabled;
}

void jr_brain_get_status(jr_brain_status_t *out) {
    if (!out) return;
    portENTER_CRITICAL(&brain_lock);
    *out = (jr_brain_status_t){
        .enabled = brain_enabled,
        .listening = brain_state == JR_BRAIN_LISTENING,
        .mic_ready = false,
        .state = brain_state,
        .triggers = brain_triggers,
        .frames = brain_frames,
        .last_level = brain_last_level,
        .last_probability = brain_last_probability,
        .last_error = brain_last_error,
    };
    snprintf(out->engine, sizeof(out->engine), "%s", "bench_stub");
    snprintf(out->last_event, sizeof(out->last_event), "%s", brain_last_event);
    portEXIT_CRITICAL(&brain_lock);
}

esp_err_t jr_brain_trigger_test(void) {
    if (!jr_brain_enabled()) return ESP_ERR_INVALID_STATE;

    portENTER_CRITICAL(&brain_lock);
    brain_triggers++;
    brain_last_probability = 1.0f;
    brain_last_error = ESP_OK;
    portEXIT_CRITICAL(&brain_lock);
    set_event("wakeword_detected");

    ESP_LOGI(TAG, "JR_BRAIN event=wakeword_detected source=bench keyword=JrBot confidence=1.000 trigger=%lu",
             (unsigned long)brain_triggers);

    if (!jr_face_set_expression("feliz")) {
        portENTER_CRITICAL(&brain_lock);
        brain_last_error = ESP_FAIL;
        brain_state = JR_BRAIN_ERROR;
        portEXIT_CRITICAL(&brain_lock);
        set_event("face_error");
        ESP_LOGE(TAG, "JR_BRAIN event=face_error expression=feliz");
        return ESP_FAIL;
    }

    jr_face_increment_command_count();
    set_event("reaction_happy");
    ESP_LOGI(TAG, "JR_BRAIN event=reaction_happy expression=feliz");
    ESP_LOGI(TAG, "JR_BRAIN event=listening wakeword=JrBot");
    return ESP_OK;
}
