#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "jr_audio.h"
#include "jr_reply_audio.h"

#define JR_REPLY_RATE 16000
#define JR_REPLY_DURATION_MS 420
#define JR_REPLY_SAMPLES ((JR_REPLY_RATE * JR_REPLY_DURATION_MS) / 1000)
#define JR_REPLY_PI 3.14159265358979323846f

static float gaussian(float x, float center, float width) {
    float d = (x - center) / width;
    return expf(-(d * d));
}

static float envelope(size_t i, size_t count) {
    float p = (float)i / (float)(count - 1);
    float attack = p < 0.08f ? p / 0.08f : 1.0f;
    float release = p > 0.88f ? (1.0f - p) / 0.12f : 1.0f;
    if (release < 0.0f) release = 0.0f;
    return attack * release;
}

esp_err_t jr_reply_play_oi(void) {
    int16_t *pcm = malloc((size_t)JR_REPLY_SAMPLES * sizeof(int16_t));
    if (!pcm) return ESP_ERR_NO_MEM;

    for (size_t i = 0; i < JR_REPLY_SAMPLES; ++i) {
        float p = (float)i / (float)(JR_REPLY_SAMPLES - 1);
        // "Oi" robotico: inicia no /o/ e desliza para /i/.
        float glide = p < 0.45f ? 0.0f : (p - 0.45f) / 0.55f;
        if (glide > 1.0f) glide = 1.0f;
        float f1 = 520.0f + (300.0f - 520.0f) * glide;
        float f2 = 900.0f + (2250.0f - 900.0f) * glide;
        float f3 = 2500.0f + (3000.0f - 2500.0f) * glide;
        float f0 = 155.0f + 18.0f * p;
        float t = (float)i / (float)JR_REPLY_RATE;
        float sample = 0.0f;
        float norm = 0.0f;

        for (int h = 1; h <= 24; ++h) {
            float frequency = f0 * (float)h;
            if (frequency >= 7600.0f) break;
            float resonance =
                1.20f * gaussian(frequency, f1, 150.0f) +
                0.95f * gaussian(frequency, f2, 230.0f) +
                0.38f * gaussian(frequency, f3, 320.0f) +
                0.04f;
            float amplitude = resonance / (float)h;
            sample += sinf(2.0f * JR_REPLY_PI * frequency * t) * amplitude;
            norm += amplitude;
        }

        if (norm > 0.001f) sample /= norm;
        sample *= envelope(i, JR_REPLY_SAMPLES);
        // Pequena queda entre os fonemas para dar articulacao ao "Oi".
        if (p > 0.38f && p < 0.48f) sample *= 0.72f;

        int32_t value = (int32_t)(sample * 24500.0f);
        if (value > 32767) value = 32767;
        if (value < -32768) value = -32768;
        pcm[i] = (int16_t)value;
    }

    esp_err_t err = jr_audio_play_pcm16_mono(pcm, JR_REPLY_SAMPLES, JR_REPLY_RATE);
    free(pcm);
    return err;
}
