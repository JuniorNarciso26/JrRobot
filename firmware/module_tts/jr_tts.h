#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#define JR_TTS_MAX_TEXT_BYTES 1200U

/* JrBot v2 - optional offline vTTS voice. Manual speech starts OFF after boot.
 * An explicitly detected WakeNet keyword may trigger one short acknowledgement,
 * while the microphone is closed; Live and other autonomous speech remain blocked. */
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
    uint32_t last_first_audio_ms; /* Request to first successful I2S write; not acoustic timing. */
    uint32_t last_total_ms;
    uint32_t last_underruns; /* FIFO starvation episodes after playback started. */
    float last_rtf;
    esp_err_t last_error;
} jr_tts_status_t;

void jr_tts_init(void);
void jr_tts_get_status(jr_tts_status_t *out);
esp_err_t jr_tts_configure(const jr_tts_settings_t *settings);
esp_err_t jr_tts_say(const char *utf8); /* Queues bounded text, never blocks HTTP. */
/* Called ONLY by WakeNet's task after releasing I2S; blocks until playback ends. */
esp_err_t jr_tts_reply_to_wake(const char *utf8);
/* Asynchronously pre-synthesize the fixed wake reply in PSRAM, at most once per boot. */
void jr_tts_prepare_wake_reply(void);
