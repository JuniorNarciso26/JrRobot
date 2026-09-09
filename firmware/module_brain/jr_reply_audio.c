#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "jr_audio.h"
#include "jr_reply_audio.h"

#define JR_REPLY_RATE 16000
#define JR_REPLY_DURATION_MS 420
#define JR_REPLY_SAMPLES ((JR_REPLY_RATE * JR_REPLY_DURATION_MS) / 1000)
#define JR_REPLY_PI 3.14159265358979323846f
#define JR_REPLY_PREPARE_STACK 4096

typedef enum {
    JR_REPLY_CACHE_IDLE = 0,
    JR_REPLY_CACHE_PREPARING,
    JR_REPLY_CACHE_READY,
    JR_REPLY_CACHE_FAILED,
} jr_reply_cache_state_t;

static const char *TAG = "jrbot_reply";
static portMUX_TYPE reply_lock = portMUX_INITIALIZER_UNLOCKED;
static jr_reply_cache_state_t cache_state = JR_REPLY_CACHE_IDLE;
static bool prepare_task_started = false;
static int16_t *cached_pcm = NULL;
static esp_err_t cache_error = ESP_ERR_INVALID_STATE;

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

static void generate_oi(int16_t *pcm) {
    for (size_t i = 0; i < JR_REPLY_SAMPLES; ++i) {
        float p = (float)i / (float)(JR_REPLY_SAMPLES - 1);
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
        if (p > 0.38f && p < 0.48f) sample *= 0.72f;

        int32_t value = (int32_t)(sample * 24500.0f);
        if (value > 32767) value = 32767;
        if (value < -32768) value = -32768;
        pcm[i] = (int16_t)value;
    }
}

static esp_err_t prepare_cache(void) {
    bool owner = false;

    for (;;) {
        portENTER_CRITICAL(&reply_lock);
        if (cache_state == JR_REPLY_CACHE_READY) {
            portEXIT_CRITICAL(&reply_lock);
            return ESP_OK;
        }
        if (cache_state == JR_REPLY_CACHE_FAILED) {
            esp_err_t err = cache_error;
            portEXIT_CRITICAL(&reply_lock);
            return err;
        }
        if (cache_state == JR_REPLY_CACHE_IDLE) {
            cache_state = JR_REPLY_CACHE_PREPARING;
            owner = true;
        }
        portEXIT_CRITICAL(&reply_lock);
        if (owner) break;
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    int64_t begin_us = esp_timer_get_time();
    size_t bytes = (size_t)JR_REPLY_SAMPLES * sizeof(int16_t);
    int16_t *pcm = (int16_t *)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!pcm) pcm = (int16_t *)malloc(bytes);
    if (!pcm) {
        portENTER_CRITICAL(&reply_lock);
        cache_error = ESP_ERR_NO_MEM;
        cache_state = JR_REPLY_CACHE_FAILED;
        portEXIT_CRITICAL(&reply_lock);
        ESP_LOGE(TAG, "reply cache allocation failed bytes=%u", (unsigned)bytes);
        return ESP_ERR_NO_MEM;
    }

    generate_oi(pcm);
    uint32_t prepare_ms = (uint32_t)((esp_timer_get_time() - begin_us) / 1000LL);

    portENTER_CRITICAL(&reply_lock);
    cached_pcm = pcm;
    cache_error = ESP_OK;
    cache_state = JR_REPLY_CACHE_READY;
    portEXIT_CRITICAL(&reply_lock);

    ESP_LOGI(TAG, "reply cache ready samples=%d bytes=%u prepare_ms=%lu",
             JR_REPLY_SAMPLES, (unsigned)bytes, (unsigned long)prepare_ms);
    return ESP_OK;
}

static void prepare_task(void *arg) {
    (void)arg;
    (void)prepare_cache();
    portENTER_CRITICAL(&reply_lock);
    prepare_task_started = false;
    portEXIT_CRITICAL(&reply_lock);
    vTaskDelete(NULL);
}

void jr_reply_prepare_oi_async(void) {
    portENTER_CRITICAL(&reply_lock);
    bool skip = prepare_task_started || cache_state == JR_REPLY_CACHE_READY ||
                cache_state == JR_REPLY_CACHE_PREPARING || cache_state == JR_REPLY_CACHE_FAILED;
    if (!skip) prepare_task_started = true;
    portEXIT_CRITICAL(&reply_lock);
    if (skip) return;

    if (xTaskCreatePinnedToCore(prepare_task, "jr_reply_prep", JR_REPLY_PREPARE_STACK,
                                NULL, 1, NULL, 0) != pdPASS) {
        portENTER_CRITICAL(&reply_lock);
        prepare_task_started = false;
        portEXIT_CRITICAL(&reply_lock);
        ESP_LOGW(TAG, "reply cache background task unavailable; will prepare on first use");
    }
}

bool jr_reply_oi_ready(void) {
    portENTER_CRITICAL(&reply_lock);
    bool ready = cache_state == JR_REPLY_CACHE_READY && cached_pcm != NULL;
    portEXIT_CRITICAL(&reply_lock);
    return ready;
}

esp_err_t jr_reply_play_oi(void) {
    int64_t begin_us = esp_timer_get_time();
    esp_err_t err = prepare_cache();
    if (err != ESP_OK) return err;

    portENTER_CRITICAL(&reply_lock);
    const int16_t *pcm = cached_pcm;
    portEXIT_CRITICAL(&reply_lock);
    if (!pcm) return ESP_ERR_INVALID_STATE;

    uint32_t cache_wait_ms = (uint32_t)((esp_timer_get_time() - begin_us) / 1000LL);
    ESP_LOGI(TAG, "reply dispatch cache_ready=1 cache_wait_ms=%lu", (unsigned long)cache_wait_ms);
    return jr_audio_play_pcm16_mono(pcm, JR_REPLY_SAMPLES, JR_REPLY_RATE);
}
