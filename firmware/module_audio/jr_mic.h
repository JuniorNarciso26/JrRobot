#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

typedef struct {
    bool present;
    unsigned samples;
    uint32_t peak_raw;
    unsigned changes;
    char channel;
    esp_err_t last_error;
} jr_mic_status_t;

bool jr_mic_probe(jr_mic_status_t *out);
bool jr_mic_test(char *response, size_t capacity);
