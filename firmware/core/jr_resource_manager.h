#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

typedef enum {
    JR_I2S_OWNER_NONE = 0,
    JR_I2S_OWNER_LIVE,
    JR_I2S_OWNER_MIC,
    JR_I2S_OWNER_PLAYBACK,
    JR_I2S_OWNER_LEGACY_VOICE,
} jr_i2s_owner_t;

typedef struct {
    jr_i2s_owner_t owner;
    uint32_t acquisitions;
    uint32_t releases;
    uint32_t busy_count;
    uint32_t release_mismatch_count;
} jr_i2s_resource_status_t;

esp_err_t jr_resource_manager_init(void);
bool jr_resource_i2s_acquire(jr_i2s_owner_t owner, uint32_t timeout_ms, const char *reason);
void jr_resource_i2s_release(jr_i2s_owner_t owner, const char *reason);
void jr_resource_i2s_get_status(jr_i2s_resource_status_t *out);
const char *jr_i2s_owner_name(jr_i2s_owner_t owner);
