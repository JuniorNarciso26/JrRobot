#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_http_server.h"

typedef struct {
    bool present;
    unsigned samples;
    uint32_t peak_raw;
    unsigned changes;
    char channel;
    esp_err_t last_error;
} jr_mic_status_t;

typedef struct {
    bool recording;
    bool has_recording;
    unsigned seconds;
    size_t samples;
    int level;
    int peak;
    char channel;
    esp_err_t last_error;
} jr_mic_recording_info_t;

bool jr_mic_probe(jr_mic_status_t *out);
bool jr_mic_test(char *response, size_t capacity);
bool jr_mic_status(char *response, size_t capacity);
esp_err_t jr_mic_record_wav_handler(httpd_req_t *req);
esp_err_t jr_mic_play_last_recording(void);
void jr_mic_get_recording_info(jr_mic_recording_info_t *out);
