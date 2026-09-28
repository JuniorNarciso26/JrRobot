#include <stdbool.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "jr_mode_manager.h"

static const char *TAG = "jrbot_mode";
static portMUX_TYPE mode_lock = portMUX_INITIALIZER_UNLOCKED;

static jr_mode_status_t mode_status = {
    .current = JR_MODE_IDLE,
    .previous = JR_MODE_IDLE,
    .transitions = 0,
};

const char *jr_mode_name(jr_mode_t mode)
{
    switch (mode) {
        case JR_MODE_IDLE: return "idle";
        case JR_MODE_AUTONOMOUS: return "autonomous";
        case JR_MODE_LIVE: return "live";
        case JR_MODE_RECORDING: return "recording";
        case JR_MODE_PLAYBACK: return "playback";
        case JR_MODE_DIAGNOSTIC: return "diagnostic";
        default: return "unknown";
    }
}

jr_mode_t jr_mode_get(void)
{
    portENTER_CRITICAL(&mode_lock);
    jr_mode_t current = mode_status.current;
    portEXIT_CRITICAL(&mode_lock);
    return current;
}

jr_mode_t jr_mode_set(jr_mode_t next, const char *reason)
{
    jr_mode_t previous;
    uint32_t transitions;
    bool changed = false;

    portENTER_CRITICAL(&mode_lock);
    previous = mode_status.current;
    if (previous != next) {
        mode_status.previous = previous;
        mode_status.current = next;
        mode_status.transitions++;
        changed = true;
    }
    transitions = mode_status.transitions;
    portEXIT_CRITICAL(&mode_lock);

    if (changed) {
        ESP_LOGI(TAG,
                 "JR_MODE_MANAGER from=%s to=%s reason=%s transitions=%lu",
                 jr_mode_name(previous),
                 jr_mode_name(next),
                 reason ? reason : "unspecified",
                 (unsigned long)transitions);
    }

    return previous;
}

void jr_mode_get_status(jr_mode_status_t *out)
{
    if (!out) return;

    portENTER_CRITICAL(&mode_lock);
    *out = mode_status;
    portEXIT_CRITICAL(&mode_lock);
}
