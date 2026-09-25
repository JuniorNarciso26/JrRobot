#include <stdbool.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "jr_resource_manager.h"

static const char *TAG = "jrbot_resource";
static portMUX_TYPE resource_lock = portMUX_INITIALIZER_UNLOCKED;
static SemaphoreHandle_t i2s_gate;

static jr_i2s_resource_status_t i2s_status = {
    .owner = JR_I2S_OWNER_NONE,
    .acquisitions = 0,
    .releases = 0,
    .busy_count = 0,
    .release_mismatch_count = 0,
};

const char *jr_i2s_owner_name(jr_i2s_owner_t owner)
{
    switch (owner) {
        case JR_I2S_OWNER_NONE: return "none";
        case JR_I2S_OWNER_LIVE: return "live";
        case JR_I2S_OWNER_MIC: return "mic";
        case JR_I2S_OWNER_PLAYBACK: return "playback";
        case JR_I2S_OWNER_LEGACY_VOICE: return "legacy_voice";
        default: return "unknown";
    }
}

esp_err_t jr_resource_manager_init(void)
{
    if (i2s_gate) return ESP_OK;

    i2s_gate = xSemaphoreCreateBinary();
    if (!i2s_gate) return ESP_ERR_NO_MEM;

    (void)xSemaphoreGive(i2s_gate);
    ESP_LOGI(TAG, "JR_RESOURCE init resource=i2s owner=none");
    return ESP_OK;
}

bool jr_resource_i2s_acquire(jr_i2s_owner_t owner, uint32_t timeout_ms, const char *reason)
{
    if (owner == JR_I2S_OWNER_NONE) return false;
    if (jr_resource_manager_init() != ESP_OK) return false;

    if (xSemaphoreTake(i2s_gate, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
        jr_i2s_owner_t current;
        uint32_t busy_count;

        portENTER_CRITICAL(&resource_lock);
        i2s_status.busy_count++;
        busy_count = i2s_status.busy_count;
        current = i2s_status.owner;
        portEXIT_CRITICAL(&resource_lock);

        ESP_LOGW(TAG,
                 "JR_RESOURCE i2s_acquire owner=%s result=busy current_owner=%s busy_count=%lu reason=%s",
                 jr_i2s_owner_name(owner),
                 jr_i2s_owner_name(current),
                 (unsigned long)busy_count,
                 reason ? reason : "unspecified");
        return false;
    }

    uint32_t acquisitions;
    portENTER_CRITICAL(&resource_lock);
    i2s_status.owner = owner;
    i2s_status.acquisitions++;
    acquisitions = i2s_status.acquisitions;
    portEXIT_CRITICAL(&resource_lock);

    ESP_LOGI(TAG,
             "JR_RESOURCE i2s_acquire owner=%s result=ok acquisitions=%lu reason=%s",
             jr_i2s_owner_name(owner),
             (unsigned long)acquisitions,
             reason ? reason : "unspecified");
    return true;
}

void jr_resource_i2s_release(jr_i2s_owner_t owner, const char *reason)
{
    if (!i2s_gate) return;

    jr_i2s_owner_t current;
    uint32_t releases = 0;
    uint32_t mismatches = 0;
    bool valid = false;

    portENTER_CRITICAL(&resource_lock);
    current = i2s_status.owner;
    if (current == owner && owner != JR_I2S_OWNER_NONE) {
        i2s_status.owner = JR_I2S_OWNER_NONE;
        i2s_status.releases++;
        releases = i2s_status.releases;
        valid = true;
    } else {
        i2s_status.release_mismatch_count++;
        mismatches = i2s_status.release_mismatch_count;
    }
    portEXIT_CRITICAL(&resource_lock);

    if (!valid) {
        ESP_LOGE(TAG,
                 "JR_RESOURCE i2s_release owner=%s result=mismatch current_owner=%s mismatch_count=%lu reason=%s",
                 jr_i2s_owner_name(owner),
                 jr_i2s_owner_name(current),
                 (unsigned long)mismatches,
                 reason ? reason : "unspecified");
        return;
    }

    (void)xSemaphoreGive(i2s_gate);
    ESP_LOGI(TAG,
             "JR_RESOURCE i2s_release owner=%s result=ok releases=%lu reason=%s",
             jr_i2s_owner_name(owner),
             (unsigned long)releases,
             reason ? reason : "unspecified");
}

void jr_resource_i2s_get_status(jr_i2s_resource_status_t *out)
{
    if (!out) return;

    portENTER_CRITICAL(&resource_lock);
    *out = i2s_status;
    portEXIT_CRITICAL(&resource_lock);
}
