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
#include "nvs.h"
#include "jr_board.h"
#include "jr_brain.h"
#include "jr_resource_manager.h"
#include "jr_tts.h"

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

#define JR_TTS_MAX_TEXT_BYTES 180U
#define JR_TTS_MAX_AUDIO_SECONDS 12
#define JR_TTS_FRAME_CAP 128
#define JR_TTS_FIRST_FRAMES 12
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
    TaskHandle_t wake_waiter;     /* WakeNet task: wait while microphone pins are released. */
} jr_tts_job_t;

typedef struct {
    vtts_model *model;
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

/* Bound synthesis to 12 seconds / max 180 UTF-8 bytes; no unbounded queues. */
static int on_pcm(const float *pcm, long n, void *opaque)
{
    jr_tts_synth_t *c = (jr_tts_synth_t *)opaque;
    if (n < 0 || c->count + (size_t)n > c->cap) {
        c->error = ESP_ERR_INVALID_SIZE;
        return 1;
    }
    for (long i = 0; i < n; ++i) {
        float v = pcm[i];
        if (v > 1.0f) v = 1.0f;
        if (v < -1.0f) v = -1.0f;
        c->pcm[c->count++] = (int16_t)lrintf(v * 32767.0f);
    }
    /* The model is CPU-heavy (~5.8 seconds observed on HW04). Periodically
     * block this worker for one RTOS tick so IDLE0 can feed its watchdog. */
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
        memset(c->pcm + c->count, 0, gap * sizeof(int16_t));
        c->count += gap;
    }
    const int first = c->segments == 1 ? JR_TTS_FIRST_FRAMES : JR_TTS_FRAME_CAP;
    long produced = vtts_synthesize_stream_ex(c->model, ids, n, c->speed,
                                               first, JR_TTS_FRAME_CAP,
                                               on_pcm, c, c->arena, c->arena_bytes);
    if (produced < 0) { c->error = ESP_FAIL; return 1; }
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

static esp_err_t play_pcm(const jr_tts_settings_t *opts, const int16_t *pcm, size_t count, int sample_rate)
{
    i2s_chan_handle_t tx = NULL;
    i2s_chan_config_t cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    cfg.auto_clear = true;
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
    for (size_t pos = 0; pos < count && err == ESP_OK;) {
        size_t frames = count - pos;
        if (frames > 256) frames = 256;
        for (size_t i = 0; i < frames; ++i) {
            float v = pcm[pos + i] / 32767.0f;
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
        pos += frames;
    }
    if (err == ESP_OK) {
        memset(stereo, 0, sizeof(stereo));
        size_t discarded = 0;
        (void)i2s_channel_write(tx, stereo, sizeof(stereo), &discarded, pdMS_TO_TICKS(300));
    }
    (void)i2s_channel_disable(tx);
    (void)i2s_del_channel(tx);
    (void)gpio_reset_pin((gpio_num_t)JR_AUDIO_BCLK_GPIO);
    (void)gpio_reset_pin((gpio_num_t)JR_AUDIO_LRC_GPIO);
    (void)gpio_set_direction((gpio_num_t)JR_AUDIO_DIN_GPIO, GPIO_MODE_OUTPUT);
    (void)gpio_set_level((gpio_num_t)JR_AUDIO_DIN_GPIO, 0);
    return err;
}

static esp_err_t synth_and_play(const jr_tts_job_t *job, uint32_t *audio_ms, uint32_t *synth_ms, float *rtf)
{
    /* A detected wakeword is the only autonomous path allowed to speak.
     * jr_voice.c closes and releases the microphone before calling this path. */
    if (jr_brain_enabled() && !job->wake_reply) return ESP_ERR_INVALID_STATE;
    if (!jr_resource_i2s_acquire(JR_I2S_OWNER_TTS, 0, "tts_speak")) return ESP_ERR_INVALID_STATE;

    esp_err_t err = ESP_ERR_NO_MEM;
    void *weights = NULL;
    vtts_model *model = NULL;
    void *arena = NULL;
    vtts_text_ws *ws = NULL;
    int16_t *pcm = NULL;

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
    size_t arena_bytes = vtts_stream_arena_bytes(model, VTTS_TEXT_MAX_IDS, JR_TTS_FRAME_CAP);
    arena = heap_caps_aligned_alloc(16, arena_bytes, MALLOC_CAP_SPIRAM);
    ws = heap_caps_malloc(sizeof(*ws), MALLOC_CAP_SPIRAM);
    size_t cap = (size_t)model->sample_rate * JR_TTS_MAX_AUDIO_SECONDS;
    pcm = heap_caps_malloc(cap * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    if (!arena || !ws || !pcm) goto cleanup;

    vtts_lex lex;
    bool has_lex = vtts_lex_init(&lex, lex_en_start, (size_t)(lex_en_end - lex_en_start)) == 0;
    vtts_text_opts opts = {.lang = VTTS_LANG_PT, .lex = has_lex ? &lex : NULL};
    jr_tts_synth_t c = {.model=model, .arena=arena, .arena_bytes=arena_bytes,
                         .pcm=pcm, .count=0, .cap=cap, .speed=job->opts.speed, .error=ESP_OK};
    int64_t started_us = esp_timer_get_time();
    int chunks = vtts_text_to_ids_ex(job->text, ws, &opts, on_ids, &c);
    int64_t synth_us = esp_timer_get_time() - started_us;
    if (c.error != ESP_OK || chunks <= 0 || c.count == 0) {
        err = c.error != ESP_OK ? c.error : ESP_FAIL;
        goto cleanup;
    }
    *audio_ms = (uint32_t)(c.count * 1000 / model->sample_rate);
    *synth_ms = (uint32_t)(synth_us / 1000);
    *rtf = (float)synth_us * model->sample_rate / (1000000.0f * (float)c.count);
    ESP_LOGI(TAG, "model=%c chunks=%d audio_ms=%lu synth_ms=%lu rtf=%.3f psram_free=%u",
             job->opts.model, chunks, (unsigned long)*audio_ms, (unsigned long)*synth_ms, (double)*rtf,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    err = play_pcm(&job->opts, &c);

cleanup:
    if (pcm) heap_caps_free(pcm);
    if (ws) heap_caps_free(ws);
    if (arena) heap_caps_free(arena);
    if (model) heap_caps_free(model);
    if (weights) heap_caps_free(weights);
    jr_resource_i2s_release(JR_I2S_OWNER_TTS, "tts_speak_done");
    return err;
}

static void voice_task(void *opaque)
{
    jr_tts_job_t *job = (jr_tts_job_t *)opaque;
    uint32_t audio_ms = 0, synth_ms = 0;
    float rtf = 0.0f;
    esp_err_t err = synth_and_play(job, &audio_ms, &synth_ms, &rtf);
    portENTER_CRITICAL(&state_lock);
    state.busy = false;
    state.last_error = err;
    state.last_audio_ms = audio_ms;
    state.last_synth_ms = synth_ms;
    state.last_rtf = rtf;
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
    size_t length = utf8 ? strlen(utf8) : 0;
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
    job->wake_reply = from_wake;
    job->wake_waiter = from_wake ? xTaskGetCurrentTaskHandle() : NULL;
    memcpy(job->text, utf8, length + 1);
    state.busy = true;
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
