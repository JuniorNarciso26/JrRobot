#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/* JrBot v2 - optional offline vTTS voice. Disabled until the owner enables it.
 * No synthesis is started during boot, Live or autonomous WakeNet operation. */
typedef struct {
    bool enabled;
    char model;           /* 's' (Lite) or 'm' (Quality). */
    int volume;           /* 0..100 */
    int gain_db;          /* 0..24, with a soft limiter */
    int debuzz_db;        /* 0..12 */
    float speed;          /* 0.7..1.6, higher is slower */
    float expression;     /* 0.5..2.5, supported by prosody models */
} jr_tts_settings_t;

typedef struct {
    bool available;      /* Engine was compiled with verified vTTS assets. */
    bool busy;
    jr_tts_settings_t settings;
    uint32_t utterances;
    uint32_t last_audio_ms;
    uint32_t last_synth_ms;
    float last_rtf;
    esp_err_t last_error;
} jr_tts_status_t;

void jr_tts_init(void);
void jr_tts_get_status(jr_tts_status_t *out);
esp_err_t jr_tts_configure(const jr_tts_settings_t *settings);
esp_err_t jr_tts_say(const char *utf8); /* Queues a short phrase, never blocks HTTP. */
