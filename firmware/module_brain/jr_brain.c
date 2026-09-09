#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "jr_brain.h"
#include "jr_face.h"
#include "jr_usb_terminal.h"
#include "jr_voice.h"

static const char *TAG = "jrbot_brain";
static portMUX_TYPE brain_lock = portMUX_INITIALIZER_UNLOCKED;
static bool brain_enabled = false;
static jr_brain_state_t brain_state = JR_BRAIN_OFF;
static uint32_t brain_triggers = 0;
static float brain_last_probability = 0.0f;
static esp_err_t brain_last_error = ESP_OK;
static char brain_last_event[32] = "boot";

static void set_event(const char *event) {
    portENTER_CRITICAL(&brain_lock);
    snprintf(brain_last_event, sizeof(brain_last_event), "%s", event ? event : "unknown");
    portEXIT_CRITICAL(&brain_lock);
}

static void emit_event(const char *line) {
    if (!line) return;
    jr_usb_terminal_emit(line);
    ESP_LOGI(TAG, "%s", line);
}

static void react_happy(const char *source, const char *keyword, const char *model_name, float probability) {
    uint32_t trigger;
    portENTER_CRITICAL(&brain_lock);
    brain_triggers++;
    brain_last_probability = probability;
    brain_last_error = ESP_OK;
    trigger = brain_triggers;
    portEXIT_CRITICAL(&brain_lock);
    set_event("wakeword_detected");

    char line[240];
    snprintf(line, sizeof(line),
             "JR_BRAIN event=wakeword_detected source=%s keyword=%s model=%s probability=%.3f trigger=%lu",
             source ? source : "unknown",
             keyword ? keyword : "unknown",
             model_name ? model_name : "unknown",
             (double)probability,
             (unsigned long)trigger);
    emit_event(line);

    if (!jr_face_set_expression("feliz")) {
        portENTER_CRITICAL(&brain_lock);
        brain_last_error = ESP_FAIL;
        brain_state = JR_BRAIN_ERROR;
        portEXIT_CRITICAL(&brain_lock);
        set_event("face_error");
        emit_event("JR_BRAIN event=face_error expression=feliz");
        return;
    }

    jr_face_increment_command_count();
    set_event("reaction_happy");
    emit_event("JR_BRAIN event=reaction_happy expression=feliz");

    jr_voice_status_t voice = {0};
    jr_voice_get_status(&voice);
    if (jr_brain_enabled() && voice.running) {
        snprintf(line, sizeof(line), "JR_BRAIN event=listening wakeword=%s model=%s",
                 voice.wakeword, voice.model);
        emit_event(line);
    }
}

static void voice_wake_detected(const char *keyword, const char *model_name, int wake_index, float probability) {
    (void)wake_index;
    react_happy("voice", keyword, model_name, probability);
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
        set_event("autonomous_starting");
        emit_event("JR_BRAIN event=autonomous_starting engine=esp-sr");

        esp_err_t err = jr_voice_start(voice_wake_detected);
        if (err != ESP_OK) {
            portENTER_CRITICAL(&brain_lock);
            brain_enabled = false;
            brain_state = JR_BRAIN_ERROR;
            brain_last_error = err;
            portEXIT_CRITICAL(&brain_lock);
            set_event("voice_start_error");
            char line[160];
            snprintf(line, sizeof(line), "JR_BRAIN event=voice_start_error error=%s", esp_err_to_name(err));
            emit_event(line);
            return err;
        }

        jr_voice_status_t voice = {0};
        jr_voice_get_status(&voice);
        portENTER_CRITICAL(&brain_lock);
        brain_state = JR_BRAIN_LISTENING;
        brain_last_error = ESP_OK;
        portEXIT_CRITICAL(&brain_lock);
        set_event("autonomous_on");

        char line[240];
        snprintf(line, sizeof(line),
                 "JR_BRAIN event=autonomous_on engine=%s model=%s wakeword=%s sample_rate=%d chunk=%d min_probability=%.2f",
                 voice.engine, voice.model, voice.wakeword, voice.sample_rate, voice.chunk_samples,
                 (double)voice.min_probability);
        emit_event(line);
        snprintf(line, sizeof(line), "JR_BRAIN event=listening wakeword=%s model=%s",
                 voice.wakeword, voice.model);
        emit_event(line);
    } else {
        set_event("autonomous_stopping");
        emit_event("JR_BRAIN event=autonomous_stopping");
        jr_voice_stop();
        portENTER_CRITICAL(&brain_lock);
        brain_state = JR_BRAIN_OFF;
        brain_last_error = ESP_OK;
        portEXIT_CRITICAL(&brain_lock);
        set_event("autonomous_off");
        emit_event("JR_BRAIN event=autonomous_off");
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

    jr_voice_status_t voice = {0};
    jr_voice_get_status(&voice);

    portENTER_CRITICAL(&brain_lock);
    jr_brain_state_t state = brain_state;
    esp_err_t last_error = brain_last_error;
    bool enabled = brain_enabled;
    uint32_t triggers = brain_triggers;
    float probability = brain_last_probability;
    char last_event[sizeof(brain_last_event)];
    snprintf(last_event, sizeof(last_event), "%s", brain_last_event);
    portEXIT_CRITICAL(&brain_lock);

    if (enabled && state == JR_BRAIN_LISTENING && !voice.running) {
        state = JR_BRAIN_ERROR;
        if (voice.last_error != ESP_OK) last_error = voice.last_error;
    }

    *out = (jr_brain_status_t){
        .enabled = enabled,
        .listening = enabled && state == JR_BRAIN_LISTENING && voice.running,
        .mic_ready = voice.mic_ready,
        .state = state,
        .triggers = triggers,
        .frames = voice.frames,
        .last_level = voice.last_level,
        .last_probability = probability,
        .last_error = last_error,
    };
    snprintf(out->engine, sizeof(out->engine), "%s", voice.engine[0] ? voice.engine : "esp-sr");
    snprintf(out->last_event, sizeof(out->last_event), "%s", last_event);
}

esp_err_t jr_brain_trigger_test(void) {
    if (!jr_brain_enabled()) return ESP_ERR_INVALID_STATE;

    react_happy("bench", "JrBot", "simulated", 1.0f);

    jr_brain_status_t status = {0};
    jr_brain_get_status(&status);
    return status.state == JR_BRAIN_ERROR ? status.last_error : ESP_OK;
}
