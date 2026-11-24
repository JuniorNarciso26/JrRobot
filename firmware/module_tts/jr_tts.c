/* JrBot - on-demand offline pt-BR speech. The optional vTTS runtime is Apache-2.0.
 * Keep model weights and buffers in PSRAM; never retain them while idle.
 * The microphone, Live and autonomous recognition keep their existing I2S owner.
 * This file is inert when JRBOT_WITH_VTTS=0 (normal 5.5 firmware build). */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"
#include "nvs.h"
#include "jr_board.h"
#include "jr_brain.h"
#include "jr_resource_manager.h"
#include "jr_tts.h"
#include "jr_tts_chunks.h"

#ifndef JRBOT_WITH_VTTS
#define JRBOT_WITH_VTTS 0
#endif

#if JRBOT_WITH_VTTS
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "vtts.h"
#include "vtts_lex.h"
#include "vtts_text.h"

extern const uint8_t model_s_start[] asm("_binary_model_s_vtts_start");
extern const uint8_t model_s_end[] asm("_binary_model_s_vtts_end");
extern const uint8_t model_m_start[] asm("_binary_model_m_vtts_start");
extern const uint8_t model_m_end[] asm("_binary_model_m_vtts_end");
extern const uint8_t lex_en_start[] asm("_binary_lex_en_bin_start");
extern const uint8_t lex_en_end[] asm("_binary_lex_en_bin_end");

int64_t vtts_now_us(void) { return esp_timer_get_time(); }
#endif

#define JR_TTS_STREAM_BYTES 32768U
#define JR_TTS_MAX_AUDIO_SECONDS 12
#define JR_TTS_FRAME_CAP 128
#define JR_TTS_FIRST_FRAMES 12
#define JR_TTS_STREAM_FRAMES 32
#define JR_WAKE_REPLY_TEXT "Olá! Eu sou o JrBot."
#define JR_WAKE_CACHE_MAX_SAMPLES (22050U * 3U)
#define JR_TTS_COOPERATIVE_YIELD_US 150000LL

static const char *TAG = "jrbot_tts";
static portMUX_TYPE state_lock = portMUX_INITIALIZER_UNLOCKED;
static jr_tts_status_t state = {
    .available = JRBOT_WITH_VTTS != 0,
    .busy = false,
    .settings = {
        .enabled = false, .model = 's', .volume = 75,
        .gain_db = 6, .debuzz_db = 0, .speed = 1.0f, .expression = 1.0f,
    },
    .last_error = ESP_OK,
};

static bool initialized = false;
#if JRBOT_WITH_VTTS
static SemaphoreHandle_t synth_mutex;
#endif

void jr_tts_init(void)
{
    if (initialized) return;
    initialized = true;
    nvs_handle_t nvs;
    if (nvs_open("jrtts", NVS_READONLY, &nvs) == ESP_OK) {
        jr_tts_settings_t c;
        portENTER_CRITICAL(&state_lock);
        c = state.settings;
        portEXIT_CRITICAL(&state_lock);
        uint8_t model = 0;
        int32_t num = 0;
        if (nvs_get_u8(nvs, "model", &model) == ESP_OK && (model == 's' || model == 'm')) c.model = (char)model;
        if (nvs_get_i32(nvs, "volume", &num) == ESP_OK && num >= 0 && num <= 100) c.volume = num;
        if (nvs_get_i32(nvs, "gain", &num) == ESP_OK && num >= 0 && num <= 24) c.gain_db = num;
        if (nvs_get_i32(nvs, "debuzz", &num) == ESP_OK && num >= 0 && num <= 12) c.debuzz_db = num;
        if (nvs_get_i32(nvs, "speed", &num) == ESP_OK && num >= 700 && num <= 1600) c.speed = num / 1000.0f;
        if (nvs_get_i32(nvs, "expr", &num) == ESP_OK && num >= 500 && num <= 2500) c.expression = num / 1000.0f;
        nvs_close(nvs);
        portENTER_CRITICAL(&state_lock);
        state.settings = c; /* Always boot with speech OFF. */
        portEXIT_CRITICAL(&state_lock);
    }
#if JRBOT_WITH_VTTS
    if (!synth_mutex) synth_mutex = xSemaphoreCreateMutex();
#endif
    ESP_LOGI(TAG, "voice=%s enabled=0 model=%c gain_db=%d",
             state.available ? "vtts_ptbr" : "not_compiled",
             state.settings.model, state.settings.gain_db);
}

void jr_tts_get_status(jr_tts_status_t *out)
{
    if (!out) return;
    portENTER_CRITICAL(&state_lock);
    *out = state;
    portEXIT_CRITICAL(&state_lock);
}

static bool valid_settings(const jr_tts_settings_t *c)
{
    return c && (c->model == 's' || c->model == 'm') &&
           c->volume >= 0 && c->volume <= 100 &&
           c->gain_db >= 0 && c->gain_db <= 24 &&
           c->debuzz_db >= 0 && c->debuzz_db <= 12 &&
           isfinite(c->speed) && c->speed >= 0.7f && c->speed <= 1.6f &&
           isfinite(c->expression) && c->expression >= 0.5f && c->expression <= 2.5f;
}

esp_err_t jr_tts_configure(const jr_tts_settings_t *settings)
{
    if (!valid_settings(settings)) return ESP_ERR_INVALID_ARG;
    if (settings->enabled && !state.available) return ESP_ERR_NOT_SUPPORTED;
    portENTER_CRITICAL(&state_lock);
    if (state.busy) {
        portEXIT_CRITICAL(&state_lock);
        return ESP_ERR_INVALID_STATE;
    }
    state.settings = *settings;
    portEXIT_CRITICAL(&state_lock);

    /* Persist tuning, never automatic speech activation. A failed NVS save
     * does not undo the runtime settings; the user can retry. */
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("jrtts", NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;
    (void)nvs_set_u8(nvs, "model", (uint8_t)settings->model);
    (void)nvs_set_i32(nvs, "volume", settings->volume);
    (void)nvs_set_i32(nvs, "gain", settings->gain_db);
    (void)nvs_set_i32(nvs, "debuzz", settings->debuzz_db);
    (void)nvs_set_i32(nvs, "speed", (int32_t)(settings->speed * 1000.0f + 0.5f));
    (void)nvs_set_i32(nvs, "expr", (int32_t)(settings->expression * 1000.0f + 0.5f));
    err = nvs_commit(nvs);
    nvs_close(nvs);
    return err;
}

#if JRBOT_WITH_VTTS
typedef struct {
    jr_tts_settings_t opts;
    char text[JR_TTS_MAX_TEXT_BYTES + 1];
    bool wake_reply;              /* Authorized only after WakeNet detected a user wakeword. */
    int64_t requested_us;
    TaskHandle_t wake_waiter;     /* WakeNet task: wait while microphone pins are released. */
} jr_tts_job_t;

/* One producer (inference) and one consumer (I2S). Only PCM storage lives in
 * PSRAM; FreeRTOS control objects remain in internal memory. Bounded independently
 * of text length. done/error are shared under state_lock. */
typedef struct {
    StreamBufferHandle_t buffer;
    StaticStreamBuffer_t control;
    uint8_t *storage;
    SemaphoreHandle_t finished;
    jr_tts_settings_t opts;
    int sample_rate;
    int64_t requested_us;
    bool done;
    esp_err_t error;
    uint32_t underruns;
    int64_t wait_us;
} jr_tts_stream_t;

typedef struct {
    vtts_model *model;
    jr_tts_stream_t *stream;
    void *arena;
    size_t arena_bytes;
    int16_t *pcm;
    size_t count;
    size_t cap;
    uint32_t segments;
    float speed;
    esp_err_t error;
    int64_t last_yield_us;
} jr_tts_synth_t;

/* This tiny reply is prepared only when Autonomous is enabled and kept in
 * PSRAM for subsequent wakewords. Never free a published cache pointer: the
 * WakeNet voice task may be playing it while the background worker exits. */
typedef struct {
    const int16_t *pcm;
    size_t count;
    int sample_rate;
    bool started;
    bool finished;
} jr_tts_wake_cache_t;
static jr_tts_wake_cache_t wake_cache = {0};

static bool cached_reply(const int16_t **pcm, size_t *count, int *sample_rate)
{
    bool ready;
    portENTER_CRITICAL(&state_lock);
    ready = wake_cache.pcm != NULL && wake_cache.count != 0;
    if (ready) {
        *pcm = wake_cache.pcm;
        *count = wake_cache.count;
        *sample_rate = wake_cache.sample_rate;
    }
    portEXIT_CRITICAL(&state_lock);
    return ready;
}

static void cache_reply_pcm(const int16_t *pcm, size_t count, int sample_rate)
{
    if (!pcm || !count || count > JR_WAKE_CACHE_MAX_SAMPLES || sample_rate != 22050) return;
    const size_t bytes = count * sizeof(int16_t);
    int16_t *copy = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!copy) {
        ESP_LOGW(TAG, "JR_TTS_WAKE_CACHE alloc_failed bytes=%u", (unsigned)bytes);
        return;
    }
    memcpy(copy, pcm, bytes);
    portENTER_CRITICAL(&state_lock);
    bool already_cached = wake_cache.pcm != NULL;
    if (!already_cached) {
        wake_cache.pcm = copy;
        wake_cache.count = count;
        wake_cache.sample_rate = sample_rate;
    }
    portEXIT_CRITICAL(&state_lock);
    if (already_cached) {
        heap_caps_free(copy);
        return;
    }
    ESP_LOGI(TAG, "JR_TTS_WAKE_CACHE_READY bytes=%u audio_ms=%u",
             (unsigned)bytes, (unsigned)(count * 1000 / (size_t)sample_rate));
}

static bool wait_for_cached_reply(void)
{
    /* If a first wake happens while prewarming, reuse the same render rather
     * than launching a second expensive synthesis. */
    for (int i = 0; i < 140; ++i) {
        const int16_t *pcm = NULL;
        size_t count = 0;
        int rate = 0;
        if (cached_reply(&pcm, &count, &rate)) return true;
        portENTER_CRITICAL(&state_lock);
        bool pending = wake_cache.started && !wake_cache.finished;
        portEXIT_CRITICAL(&state_lock);
        if (!pending) return false;
        vTaskDelay(pdMS_TO_TICKS(60));
    }
    return false;
}

static esp_err_t stream_send(jr_tts_stream_t *s, const int16_t *pcm, size_t count)
{
    const uint8_t *bytes = (const uint8_t *)pcm;
    size_t left = count * sizeof(*pcm);
    while (left) {
        portENTER_CRITICAL(&state_lock);
        esp_err_t err = s->error;
        portEXIT_CRITICAL(&state_lock);
        if (err != ESP_OK) return err;
        int64_t before = esp_timer_get_time();
        size_t sent = xStreamBufferSend(s->buffer, bytes, left, pdMS_TO_TICKS(100));
        s->wait_us += esp_timer_get_time() - before;
        bytes += sent;
        left -= sent;
    }
    return ESP_OK;
}

/* Cached wake clips keep the accepted full-buffer renderer. Manual speech
 * publishes each callback immediately; no whole-utterance PCM allocation. */
static int on_pcm(const float *pcm, long n, void *opaque)
{
    jr_tts_synth_t *c = (jr_tts_synth_t *)opaque;
    if (n < 0 || (size_t)n > c->cap - c->count) {
        c->error = ESP_ERR_INVALID_SIZE;
        return 1;
    }
    int16_t block[256];
    for (long pos = 0; pos < n;) {
        size_t frames = (size_t)(n - pos);
        if (frames > 256) frames = 256;
        for (size_t i = 0; i < frames; ++i) {
            float v = pcm[pos + (long)i];
            if (v > 1.0f) v = 1.0f;
            if (v < -1.0f) v = -1.0f;
            block[i] = (int16_t)lrintf(v * 32767.0f);
        }
        if (c->stream) {
            c->error = stream_send(c->stream, block, frames);
            if (c->error != ESP_OK) return 1;
        } else {
            memcpy(c->pcm + c->count, block, frames * sizeof(*block));
        }
        c->count += frames;
        pos += (long)frames;
    }
    int64_t now = esp_timer_get_time();
    if (!c->last_yield_us || now - c->last_yield_us >= JR_TTS_COOPERATIVE_YIELD_US) {
        c->last_yield_us = now;
        vTaskDelay(1);
    }
    return 0;
}

static int on_ids(const int32_t *ids, int n, void *opaque)
{
    jr_tts_synth_t *c = (jr_tts_synth_t *)opaque;
    if (c->segments++) {
        size_t gap = ((size_t)c->model->sample_rate * 100) / 1000;
        if (c->count + gap > c->cap) { c->error = ESP_ERR_INVALID_SIZE; return 1; }
        if (c->stream) {
            const int16_t silence[128] = {0};
            for (size_t left = gap; left;) {
                size_t take = left > 128 ? 128 : left;
                c->error = stream_send(c->stream, silence, take);
                if (c->error != ESP_OK) return 1;
                left -= take;
            }
        } else {
            memset(c->pcm + c->count, 0, gap * sizeof(int16_t));
        }
        c->count += gap;
    }
    const int first = (c->stream || c->segments == 1) ? JR_TTS_FIRST_FRAMES : JR_TTS_FRAME_CAP;
    /* Smaller steady blocks trade throughput for earlier incremental playback.
     * Leave the approved cache renderer at its original frame cap. */
    const int step = c->stream ? JR_TTS_STREAM_FRAMES : JR_TTS_FRAME_CAP;
    long produced = vtts_synthesize_stream_ex(c->model, ids, n, c->speed,
                                               first, step,
                                               on_pcm, c, c->arena, c->arena_bytes);
    if (produced < 0) { if (c->error == ESP_OK) c->error = ESP_FAIL; return 1; }
    return c->error == ESP_OK ? 0 : 1;
}

/* Relative to 0 dB: linear below -3 dBFS, soft saturation above. */
static float shape(float v)
{
    float a = fabsf(v);
    if (a <= 0.7f) return v;
    float over = (a - 0.7f) / 0.3f;
    float y = 0.7f + 0.3f * over / (1.0f + over);
    return v < 0.0f ? -y : y;
}

static esp_err_t play_pcm(const jr_tts_settings_t *opts, const int16_t *pcm, size_t count, int sample_rate,
                          jr_tts_stream_t *stream, int64_t requested_us)
{
    i2s_chan_handle_t tx = NULL;
    i2s_chan_config_t cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    cfg.auto_clear = true;
    if (stream) {
        cfg.dma_desc_num = 4;
        cfg.dma_frame_num = 256;
    }
    esp_err_t err = i2s_new_channel(&cfg, &tx, NULL);
    if (err != ESP_OK) return err;

    i2s_std_config_t std = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = JR_AUDIO_BCLK_GPIO,
            .ws = JR_AUDIO_LRC_GPIO,
            .dout = JR_AUDIO_DIN_GPIO,
            .din = I2S_GPIO_UNUSED,
        },
    };
    err = i2s_channel_init_std_mode(tx, &std);
    if (err == ESP_OK) err = i2s_channel_enable(tx);
    if (err != ESP_OK) {
        (void)i2s_del_channel(tx);
        return err;
    }

    const float amplitude = (float)opts->volume / 100.0f;
    const float gain = powf(10.0f, (float)opts->gain_db / 20.0f);
    const float hf = powf(10.0f, -(float)opts->debuzz_db / 20.0f);
    const float alpha = 1.0f - expf(-2.0f * 3.14159265f * 6000.0f / (float)sample_rate);
    float low = 0.0f;
    int16_t stereo[512];
    bool first_audio = true;
    bool starved = false;
    int16_t mono[256];
    for (size_t pos = 0; err == ESP_OK && (stream || pos < count);) {
        size_t frames;
        const int16_t *source;
        if (stream) {
            /* Read done BEFORE receive: observing done also observes the final
             * producer send. Never discard a last block racing with this read. */
            portENTER_CRITICAL(&state_lock);
            bool done = stream->done;
            portEXIT_CRITICAL(&state_lock);
            size_t received = xStreamBufferReceive(stream->buffer, mono, sizeof(mono), pdMS_TO_TICKS(40));
            if (!received) {
                if (done) break;
                if (!first_audio && !starved) stream->underruns++;
                starved = true;
                continue; /* auto_clear prevents DMA replay when inference is slower. */
            }
            starved = false;
            frames = received / sizeof(*mono);
            source = mono;
        } else {
            frames = count - pos;
            if (frames > 256) frames = 256;
            source = pcm + pos;
        }
        for (size_t i = 0; i < frames; ++i) {
            float v = source[i] / 32767.0f;
            if (opts->debuzz_db > 0) {
                low += alpha * (v - low);
                v = low + (v - low) * hf;
            }
            v = shape(v * gain) * amplitude;
            int32_t sample = (int32_t)lrintf(v * 32767.0f);
            if (sample > 32767) sample = 32767;
            if (sample < -32768) sample = -32768;
            stereo[2 * i] = stereo[2 * i + 1] = (int16_t)sample;
        }
        size_t sent = 0;
        size_t bytes = frames * 2 * sizeof(int16_t);
        err = i2s_channel_write(tx, stereo, bytes, &sent, pdMS_TO_TICKS(1500));
        if (err == ESP_OK && sent != bytes) err = ESP_ERR_INVALID_SIZE;
        if (err == ESP_OK && first_audio) {
            uint32_t elapsed = (uint32_t)((esp_timer_get_time() - requested_us) / 1000);
            portENTER_CRITICAL(&state_lock);
            state.last_first_audio_ms = elapsed;
            portEXIT_CRITICAL(&state_lock);
            ESP_LOGI(TAG, "JR_TTS_FIRST_AUDIO latency_ms=%lu progressive=%d",
                     (unsigned long)elapsed, stream != NULL);
            first_audio = false;
        }
        pos += frames;
    }
    if (err == ESP_OK) {
        memset(stereo, 0, sizeof(stereo));
        size_t discarded = 0;
        for (int i = 0; i < (stream ? 5 : 1) && err == ESP_OK; ++i) {
            err = i2s_channel_write(tx, stereo, sizeof(stereo), &discarded, pdMS_TO_TICKS(300));
        }
    }
    (void)i2s_channel_disable(tx);
    (void)i2s_del_channel(tx);
    (void)gpio_reset_pin((gpio_num_t)JR_AUDIO_BCLK_GPIO);
    (void)gpio_reset_pin((gpio_num_t)JR_AUDIO_LRC_GPIO);
    (void)gpio_set_direction((gpio_num_t)JR_AUDIO_DIN_GPIO, GPIO_MODE_OUTPUT);
    (void)gpio_set_level((gpio_num_t)JR_AUDIO_DIN_GPIO, 0);
    return err;
}

static void stream_player_task(void *opaque)
{
    jr_tts_stream_t *s = opaque;
    esp_err_t err = play_pcm(&s->opts, NULL, 0, s->sample_rate, s, s->requested_us);
    portENTER_CRITICAL(&state_lock);
    s->error = err;
    portEXIT_CRITICAL(&state_lock);
    xSemaphoreGive(s->finished);
    /* Do not access s after signalling: the producer may now free it. */
    vTaskDelete(NULL);
}

static void stream_destroy(jr_tts_stream_t *s)
{
    if (!s) return;
    if (s->buffer) vStreamBufferDelete(s->buffer);
    if (s->finished) vSemaphoreDelete(s->finished);
    if (s->storage) heap_caps_free(s->storage);
    heap_caps_free(s);
}

static jr_tts_stream_t *stream_start(const jr_tts_job_t *job, int sample_rate)
{
    jr_tts_stream_t *s = heap_caps_calloc(1, sizeof(*s), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!s) return NULL;
    s->opts = job->opts;
    s->sample_rate = sample_rate;
    s->requested_us = job->requested_us;
    s->storage = heap_caps_malloc(JR_TTS_STREAM_BYTES + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s->finished = xSemaphoreCreateBinary();
    if (s->storage) s->buffer = xStreamBufferCreateStatic(JR_TTS_STREAM_BYTES + 1, 1, s->storage, &s->control);
    if (!s->buffer || !s->finished ||
        xTaskCreatePinnedToCore(stream_player_task, "jr_tts_play", 6144, s, 5, NULL, 1) != pdPASS) {
        stream_destroy(s);
        return NULL;
    }
    return s;
}

static esp_err_t synth_and_play(const jr_tts_job_t *job, uint32_t *audio_ms,
                                uint32_t *synth_ms, float *rtf, bool cache_only)
{
    if (jr_brain_enabled() && !job->wake_reply) return ESP_ERR_INVALID_STATE;

    if (job->wake_reply && !cache_only) {
        /* The normal path uses an immutable PCM clip, generated earlier when
         * Autonomous was turned on. No model load, PSRAM arena or TTS inference
         * is needed at wake time. If prewarm is still running, reuse its work. */
        const int16_t *ready_pcm = NULL;
        size_t ready_count = 0;
        int ready_rate = 0;
        if (!cached_reply(&ready_pcm, &ready_count, &ready_rate)) {
            (void)wait_for_cached_reply();
        }
        if (cached_reply(&ready_pcm, &ready_count, &ready_rate)) {
            if (!jr_resource_i2s_acquire(JR_I2S_OWNER_TTS, 0, "tts_wake_cached"))
                return ESP_ERR_INVALID_STATE;
            *audio_ms = (uint32_t)(ready_count * 1000U / (size_t)ready_rate);
            *synth_ms = 0;
            *rtf = 0.0f;
            ESP_LOGI(TAG, "JR_TTS_WAKE_CACHE_HIT audio_ms=%lu bytes=%lu",
                     (unsigned long)*audio_ms,
                     (unsigned long)(ready_count * sizeof(int16_t)));
            esp_err_t played = play_pcm(&job->opts, ready_pcm, ready_count, ready_rate, NULL, job->requested_us);
            jr_resource_i2s_release(JR_I2S_OWNER_TTS, "tts_wake_cached_done");
            return played;
        }
        portENTER_CRITICAL(&state_lock);
        bool still_preparing = wake_cache.started && !wake_cache.finished;
        portEXIT_CRITICAL(&state_lock);
        if (still_preparing) {
            ESP_LOGW(TAG, "JR_TTS_WAKE_CACHE not_ready_timeout; preserve beep_and_face");
            return ESP_ERR_TIMEOUT;
        }
        /* Prewarm could not start or failed; retain the existing lazy vTTS
         * synthesis as a fallback, and opportunistically populate the cache. */
    }

    bool i2s_owned = false;
    bool synth_locked = false;
    esp_err_t err = ESP_ERR_NO_MEM;
    void *weights = NULL;
    vtts_model *model = NULL;
    void *arena = NULL;
    vtts_text_ws *ws = NULL;
    int16_t *pcm = NULL;
    jr_tts_stream_t *stream = NULL;
    bool progressive = !cache_only && !job->wake_reply;
    if (!cache_only) {
        if (!jr_resource_i2s_acquire(JR_I2S_OWNER_TTS, 0, "tts_speak"))
            return ESP_ERR_INVALID_STATE;
        i2s_owned = true;
    }
    if (!synth_mutex || xSemaphoreTake(synth_mutex, pdMS_TO_TICKS(15000)) != pdTRUE) {
        err = ESP_ERR_TIMEOUT;
        goto cleanup;
    }
    synth_locked = true;

    const uint8_t *start = job->opts.model == 'm' ? model_m_start : model_s_start;
    const uint8_t *end = job->opts.model == 'm' ? model_m_end : model_s_end;
    size_t bytes = (size_t)(end - start);
    const uint8_t *blob = start;
    weights = heap_caps_aligned_alloc(16, bytes, MALLOC_CAP_SPIRAM);
    if (weights) { memcpy(weights, start, bytes); blob = (const uint8_t *)weights; }

    model = heap_caps_calloc(1, sizeof(*model), MALLOC_CAP_SPIRAM);
    if (!model) goto cleanup;
    if (vtts_init(model, blob, bytes) != 0) { err = ESP_FAIL; goto cleanup; }
    if (model->sample_rate != 22050) { err = ESP_ERR_NOT_SUPPORTED; goto cleanup; }
    vtts_set_expressiveness(model, job->opts.expression);
    size_t arena_bytes = vtts_stream_arena_bytes(model, VTTS_TEXT_MAX_IDS, progressive ? JR_TTS_STREAM_FRAMES : JR_TTS_FRAME_CAP);
    arena = heap_caps_aligned_alloc(16, arena_bytes, MALLOC_CAP_SPIRAM);
    ws = heap_caps_malloc(sizeof(*ws), MALLOC_CAP_SPIRAM);
    /* Manual text may run longer than 12s; a fixed 32 KiB FIFO bounds memory.
     * The cached wake fallback retains its original 12s clip limit. */
    size_t cap = (size_t)model->sample_rate * (progressive ? 300 : JR_TTS_MAX_AUDIO_SECONDS);
    if (!progressive) pcm = heap_caps_malloc(cap * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    if (!arena || !ws || (!progressive && !pcm)) goto cleanup;
    if (progressive) {
        stream = stream_start(job, model->sample_rate);
        if (!stream) goto cleanup;
    }

    vtts_lex lex;
    bool has_lex = vtts_lex_init(&lex, lex_en_start, (size_t)(lex_en_end - lex_en_start)) == 0;
    vtts_text_opts opts = {.lang = VTTS_LANG_PT, .lex = has_lex ? &lex : NULL};
    jr_tts_synth_t c = {.model=model, .arena=arena, .arena_bytes=arena_bytes,
                         .stream=stream, .pcm=pcm, .count=0, .cap=cap, .speed=job->opts.speed,
                         .error=ESP_OK, .last_yield_us=esp_timer_get_time()};
    int64_t started_us = esp_timer_get_time();
    int chunks = 0;
    if (progressive) {
        /* Short first phrase reduces encoder work before the first PCM block.
         * Reuse model/arena/lexicon across subsequent text parts. */
        char part[JR_TTS_MAX_TEXT_BYTES + 1];
        for (size_t pos = 0; job->text[pos] && c.error == ESP_OK;) {
            size_t length = jr_tts_chunk_bytes(job->text + pos, pos ? 128 : 64);
            memcpy(part, job->text + pos, length);
            part[length] = '\0';
            int emitted = vtts_text_to_ids_ex(part, ws, &opts, on_ids, &c);
            if (emitted < 0) { if (c.error == ESP_OK) c.error = ESP_FAIL; break; }
            chunks += emitted;
            pos += length;
        }
    } else {
        chunks = vtts_text_to_ids_ex(job->text, ws, &opts, on_ids, &c);
    }
    int64_t synth_us = esp_timer_get_time() - started_us - (stream ? stream->wait_us : 0);
    if (c.error != ESP_OK || chunks <= 0 || c.count == 0) {
        err = c.error != ESP_OK ? c.error : ESP_FAIL;
        goto cleanup;
    }
    *audio_ms = (uint32_t)(c.count * 1000U / (size_t)model->sample_rate);
    *synth_ms = (uint32_t)(synth_us / 1000);
    *rtf = (float)synth_us * model->sample_rate / (1000000.0f * (float)c.count);
    ESP_LOGI(TAG, "model=%c chunks=%d audio_ms=%lu synth_ms=%lu rtf=%.3f psram_free=%u cache_only=%d",
             job->opts.model, chunks, (unsigned long)*audio_ms, (unsigned long)*synth_ms,
             (double)*rtf, (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             cache_only ? 1 : 0);

    if (cache_only || job->wake_reply) {
        cache_reply_pcm(c.pcm, c.count, model->sample_rate);
    }
    if (cache_only) {
        const int16_t *ready_pcm = NULL;
        size_t ready_count = 0;
        int ready_rate = 0;
        err = cached_reply(&ready_pcm, &ready_count, &ready_rate) ? ESP_OK : ESP_ERR_NO_MEM;
    } else if (stream) {
        err = ESP_OK; /* Consumer is already playing. Cleanup joins it. */
    } else {
        err = play_pcm(&job->opts, c.pcm, c.count, model->sample_rate, NULL, job->requested_us);
    }

cleanup:
    if (stream) {
        portENTER_CRITICAL(&state_lock);
        stream->done = true;
        portEXIT_CRITICAL(&state_lock);
        /* Join before releasing I2S, busy or any consumer-owned memory. */
        (void)xSemaphoreTake(stream->finished, portMAX_DELAY);
        if (err == ESP_OK) err = stream->error;
        portENTER_CRITICAL(&state_lock);
        state.last_underruns = stream->underruns;
        portEXIT_CRITICAL(&state_lock);
        ESP_LOGI(TAG, "JR_TTS_STREAM_DONE underruns=%lu result=%s",
                 (unsigned long)stream->underruns, esp_err_to_name(err));
        stream_destroy(stream);
    }
    if (pcm) heap_caps_free(pcm);
    if (ws) heap_caps_free(ws);
    if (arena) heap_caps_free(arena);
    if (model) heap_caps_free(model);
    if (weights) heap_caps_free(weights);
    if (synth_locked) xSemaphoreGive(synth_mutex);
    if (i2s_owned) jr_resource_i2s_release(JR_I2S_OWNER_TTS, "tts_speak_done");
    return err;
}

static void wake_cache_task(void *opaque)
{
    (void)opaque;
    /* Lightweight acknowledgement: always use Lite S, speed 1.0, expression
     * 1.0. Playback still respects the user's current volume/gain/filter. */
    jr_tts_status_t snapshot = {0};
    jr_tts_get_status(&snapshot);
    jr_tts_job_t job = {.opts=snapshot.settings, .wake_reply=true};
    job.opts.model = 's';
    job.opts.speed = 1.0f;
    job.opts.expression = 1.0f;
    snprintf(job.text, sizeof(job.text), "%s", JR_WAKE_REPLY_TEXT);
    uint32_t audio_ms = 0, synth_ms = 0;
    float rtf = 0.0f;
    ESP_LOGI(TAG, "JR_TTS_WAKE_CACHE_PREPARE start model=s");
    esp_err_t err = synth_and_play(&job, &audio_ms, &synth_ms, &rtf, true);
    portENTER_CRITICAL(&state_lock);
    wake_cache.finished = true;
    portEXIT_CRITICAL(&state_lock);
    ESP_LOGI(TAG, "JR_TTS_WAKE_CACHE_PREPARE done=%s synth_ms=%lu rtf=%.3f",
             esp_err_to_name(err), (unsigned long)synth_ms, (double)rtf);
    vTaskDelete(NULL);
}

static void voice_task(void *opaque)
{
    jr_tts_job_t *job = (jr_tts_job_t *)opaque;
    uint32_t audio_ms = 0, synth_ms = 0;
    float rtf = 0.0f;
    esp_err_t err = synth_and_play(job, &audio_ms, &synth_ms, &rtf, false);
    portENTER_CRITICAL(&state_lock);
    state.busy = false;
    state.last_error = err;
    state.last_audio_ms = audio_ms;
    state.last_synth_ms = synth_ms;
    state.last_rtf = rtf;
    state.last_total_ms = (uint32_t)((esp_timer_get_time() - job->requested_us) / 1000);
    if (err == ESP_OK) state.utterances++;
    portEXIT_CRITICAL(&state_lock);
    ESP_LOGI(TAG, "JR_TTS done source=%s result=%s",
             job->wake_reply ? "wake" : "manual", esp_err_to_name(err));
    TaskHandle_t wake_waiter = job->wake_waiter;
    free(job);
    if (wake_waiter) xTaskNotifyGive(wake_waiter);  /* I2S has been released. */
    vTaskDelete(NULL);
}
#endif

static esp_err_t start_speech(const char *utf8, bool from_wake)
{
    size_t length = utf8 ? strnlen(utf8, JR_TTS_MAX_TEXT_BYTES + 1) : 0;
    if (!length || length > JR_TTS_MAX_TEXT_BYTES) return ESP_ERR_INVALID_ARG;
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)utf8[i];
        if (c < 32 && c != '\n' && c != '\t') return ESP_ERR_INVALID_ARG;
    }
#if JRBOT_WITH_VTTS
    if (jr_brain_enabled() && !from_wake) return ESP_ERR_INVALID_STATE;
    jr_tts_job_t *job = malloc(sizeof(*job));
    if (!job) return ESP_ERR_NO_MEM;
    portENTER_CRITICAL(&state_lock);
    /* Manual TTS still boots OFF. A physical WakeNet trigger explicitly
     * authorizes this one short reply, independently of the manual switch. */
    if ((!state.settings.enabled && !from_wake) || state.busy) {
        portEXIT_CRITICAL(&state_lock);
        free(job);
        return ESP_ERR_INVALID_STATE;
    }
    job->opts = state.settings;
    job->requested_us = esp_timer_get_time();
    job->wake_reply = from_wake;
    job->wake_waiter = from_wake ? xTaskGetCurrentTaskHandle() : NULL;
    memcpy(job->text, utf8, length + 1);
    state.busy = true;
    state.last_first_audio_ms = 0;
    state.last_total_ms = 0;
    state.last_underruns = 0;
    portEXIT_CRITICAL(&state_lock);
    /* vTTS upstream parallel worker is pinned to core 1; run synthesis caller
     * on core 0 so its two-core kernels do not contend on the same core. */
    BaseType_t created = xTaskCreatePinnedToCore(voice_task, "jr_tts", 10240, job, 5, NULL, 0);
    if (created != pdPASS) {
        portENTER_CRITICAL(&state_lock);
        state.busy = false;
        portEXIT_CRITICAL(&state_lock);
        free(job);
        return ESP_ERR_NO_MEM;
    }
    if (from_wake) {
        /* Only jr_voice's core-1 task uses this path. The vTTS worker runs on
         * core 0, allowing the upstream core-1 inference worker to run.
         * Keep the microphone closed until synthesis and playback complete. */
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        jr_tts_status_t result = {0};
        jr_tts_get_status(&result);
        return result.last_error;
    }
    return ESP_OK;
#else
    (void)from_wake;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t jr_tts_say(const char *utf8)
{
    return start_speech(utf8, false);
}

esp_err_t jr_tts_reply_to_wake(const char *utf8)
{
    return start_speech(utf8, true);
}

void jr_tts_prepare_wake_reply(void)
{
#if JRBOT_WITH_VTTS
    if (!initialized) jr_tts_init();
    portENTER_CRITICAL(&state_lock);
    bool start = !wake_cache.started;
    if (start) wake_cache.started = true;
    portEXIT_CRITICAL(&state_lock);
    if (!start) return;  /* At most one generation per boot. */

    /* Prepare on CPU0 without acquiring I2S. WakeNet listens on CPU1. The
     * low-priority task yields periodically to keep Wi-Fi and IDLE0 alive. */
    if (xTaskCreatePinnedToCore(wake_cache_task, "jr_tts_cache", 10240,
                                NULL, 1, NULL, 0) != pdPASS) {
        portENTER_CRITICAL(&state_lock);
        wake_cache.finished = true;
        portEXIT_CRITICAL(&state_lock);
        ESP_LOGW(TAG, "JR_TTS_WAKE_CACHE_PREPARE task_create_failed");
    }
#endif
}
