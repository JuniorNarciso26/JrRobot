#pragma once
#include <stddef.h>
#include "esp_err.h"

/* Caller owns the returned PSRAM buffer and must heap_caps_free() it. */
esp_err_t jr_memory_full_json(char **out, size_t *length);
