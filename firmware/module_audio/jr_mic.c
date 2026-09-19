#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "jr_audio.h"
#include "jr_board.h"
#include "jr_mic.h"

#define JR_MIC_SAMPLE_RATE 16000
#define JR_MIC_FRAMES 256
#define JR_MIC_MAX_RECORD_SECONDS 10
#define JR_MIC_PROBE_BUS_TIMEOUT_MS 1200
#define JR_MIC_RECORD_BUS_TIMEOUT_MS 15000
#define JR_MIC_STREAM_CHUNK_MS 80
#define JR_MIC_STREAM_SAMPLES ((JR_MIC_SAMPLE_RATE * JR_MIC_STREAM_CHUNK_MS) / 1000)
#define JR_MIC_STREAM_TASK_STACK 8192

static const char *TAG = "jrbot_mic";
static int16_t *last_recording = NULL;
static size_t last_recording_capacity = 0;
static size_t last_recording_samples = 0;
static unsigned last_recording_seconds = 0;
static int last_level = 0;
static int last_peak = 0;
static char last_channel = 'L';
static bool recording_active = false;
static esp_err_t last_record_error = ESP_ERR_INVALID_STATE;
static uint32_t live_request_count = 0;
static uint32_t live_ok_count = 0;
static uint32_t live_error_count = 0;
static uint32_t live_busy_count = 0;
static uint64_t live_bytes_sent = 0;
static uint32_t live_last_capture_ms = 0;
static uint32_t live_max_capture_ms = 0;
static uint32_t live_last_block_ms = 0;
static uint32_t live_stream_count = 0;
static uint32_t live_stream_chunks = 0;
static uint32_t live_stream_errors = 0;
static uint64_t live_stream_bytes = 0;
static uint32_t live_stream_last_chunk_ms = 0;
static uint32_t live_stream_max_chunk_ms = 0;
static bool live_stream_active = false;
static char live_session[32] = "none";
static portMUX_TYPE live_stream_lock = portMUX_INITIALIZER_UNLOCKED;

typedef struct {
    httpd_req_t *req;
    char session[32];
} jr_mic_stream_ctx_t;

static uint32_t magnitude32(int32_t value) {
    int64_t v = value;
    if (v < 0) v = -v;
    return (uint32_t)v;
}

static uint32_t difference32(int32_t a, int32_t b) {
    int64_t d = (int64_t)a - (int64_t)b;
    if (d < 0) d = -d;
    return d > UINT32_MAX ? UINT32_MAX : (uint32_t)d;
}

static int abs16(int16_t value) {
    int v = value;
    return v < 0 ? -v : v;
}

static void write_le16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xff);
    p[1] = (uint8_t)((v >> 8) & 0xff);
}

static void write_le32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xff);
    p[1] = (uint8_t)((v >> 8) & 0xff);
    p[2] = (uint8_t)((v >> 16) & 0xff);
    p[3] = (uint8_t)((v >> 24) & 0xff);
}

static void make_wav_header(uint8_t header[44], uint32_t sample_rate, uint32_t data_bytes) {
    memcpy(header + 0, "RIFF", 4);
    write_le32(header + 4, 36 + data_bytes);
    memcpy(header + 8, "WAVE", 4);
    memcpy(header + 12, "fmt ", 4);
    write_le32(header + 16, 16);
    write_le16(header + 20, 1);
    write_le16(header + 22, 1);
    write_le32(header + 24, sample_rate);
    write_le32(header + 28, sample_rate * 2);
    write_le16(header + 32, 2);
    write_le16(header + 34, 16);
    memcpy(header + 36, "data", 4);
    write_le32(header + 40, data_bytes);
}

static void quiet_amplifier(void) {
    (void)gpio_set_direction((gpio_num_t)JR_AUDIO_DIN_GPIO, GPIO_MODE_OUTPUT);
    (void)gpio_set_level((gpio_num_t)JR_AUDIO_DIN_GPIO, 0);
}

static void release_mic_pins(void) {
    (void)gpio_reset_pin((gpio_num_t)JR_MIC_SCK_GPIO);
    (void)gpio_set_pull_mode((gpio_num_t)JR_MIC_SCK_GPIO, GPIO_FLOATING);
    (void)gpio_set_direction((gpio_num_t)JR_MIC_SCK_GPIO, GPIO_MODE_INPUT);
    (void)gpio_reset_pin((gpio_num_t)JR_MIC_WS_GPIO);
    (void)gpio_set_pull_mode((gpio_num_t)JR_MIC_WS_GPIO, GPIO_FLOATING);
    (void)gpio_set_direction((gpio_num_t)JR_MIC_WS_GPIO, GPIO_MODE_INPUT);
    (void)gpio_set_pull_mode((gpio_num_t)JR_MIC_SD_GPIO, GPIO_FLOATING);
    (void)gpio_set_direction((gpio_num_t)JR_MIC_SD_GPIO, GPIO_MODE_INPUT);
    quiet_amplifier();
}

static esp_err_t mic_rx_open(i2s_chan_handle_t *out) {
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = NULL;
    quiet_amplifier();
    (void)gpio_set_pull_mode((gpio_num_t)JR_MIC_SD_GPIO, GPIO_PULLDOWN_ONLY);

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    esp_err_t err = i2s_new_channel(&chan_cfg, NULL, out);
    if (err != ESP_OK) return err;

    i2s_std_config_t cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(JR_MIC_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = JR_MIC_SCK_GPIO,
            .ws = JR_MIC_WS_GPIO,
            .dout = I2S_GPIO_UNUSED,
            .din = JR_MIC_SD_GPIO,
            .invert_flags = {.mclk_inv=false,.bclk_inv=false,.ws_inv=false},
        },
    };
    cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    err = i2s_channel_init_std_mode(*out, &cfg);
    if (err != ESP_OK) return err;
    return i2s_channel_enable(*out);
}

static void mic_rx_close(i2s_chan_handle_t rx) {
    if (rx) {
        (void)i2s_channel_disable(rx);
        (void)i2s_del_channel(rx);
    }
    release_mic_pins();
}

bool jr_mic_probe(jr_mic_status_t *out) {
    jr_mic_status_t result = {.present=false,.samples=0,.peak_raw=0,.changes=0,.channel='L',.last_error=ESP_ERR_INVALID_STATE};
    if (!JR_MIC_ENABLED) {
        result.last_error = ESP_ERR_NOT_SUPPORTED;
        if (out) *out = result;
        return false;
    }
    if (!jr_audio_bus_acquire(JR_MIC_PROBE_BUS_TIMEOUT_MS)) {
        result.last_error = ESP_ERR_TIMEOUT;
        if (out) *out = result;
        return false;
    }

    jr_audio_stop();
    i2s_chan_handle_t rx = NULL;
    esp_err_t err = mic_rx_open(&rx);
    if (err != ESP_OK) goto finish;

    int32_t samples[JR_MIC_FRAMES];
    memset(samples, 0, sizeof(samples));
    size_t bytes_read = 0;
    err = i2s_channel_read(rx, samples, sizeof(samples), &bytes_read, pdMS_TO_TICKS(500));
    if (err != ESP_OK) goto finish;

    unsigned frames = (unsigned)(bytes_read / sizeof(int32_t));
    uint32_t peak = 0;
    unsigned nonzero = 0, changes = 0;
    int32_t prev = 0;
    for (unsigned f=0; f<frames; ++f) {
        int32_t v = samples[f];
        uint32_t mag = magnitude32(v);
        if (mag > peak) peak = mag;
        if (mag > 16) nonzero++;
        if (f && difference32(v,prev) > 16) changes++;
        prev = v;
    }
    result.samples = frames;
    result.peak_raw = peak;
    result.changes = changes;
    result.channel = 'L';
    result.present = frames >= 32 && peak > 256 && nonzero >= 8 && changes >= 4;
    result.last_error = ESP_OK;

finish:
    mic_rx_close(rx);
    if (err != ESP_OK) result.last_error = err;
    jr_audio_bus_release();
    if (out) *out = result;
    return result.present;
}

bool jr_mic_test(char *response, size_t capacity) {
    if (!response || capacity < 160) return false;
    jr_mic_status_t s;
    bool present = jr_mic_probe(&s);
    if (s.last_error != ESP_OK) {
        snprintf(response, capacity, "JR_ERROR mic_test=io_error error=%s model=%s sck=%d ws=%d sd=%d",
                 esp_err_to_name(s.last_error),JR_MIC_MODEL,JR_MIC_SCK_GPIO,JR_MIC_WS_GPIO,JR_MIC_SD_GPIO);
        return false;
    }
    snprintf(response, capacity,
             present ? "JR_OK mic_test=signal_detected model=%s channel=%c samples=%u peak_raw=%lu changes=%u"
                     : "JR_ERROR mic_test=no_signal model=%s channel=%c samples=%u peak_raw=%lu changes=%u check=VDD_GND_SCK_WS_SD_LR",
             JR_MIC_MODEL,s.channel,s.samples,(unsigned long)s.peak_raw,s.changes);
    return present;
}

void jr_mic_get_recording_info(jr_mic_recording_info_t *out) {
    if (!out) return;
    *out = (jr_mic_recording_info_t){
        .recording = recording_active,
        .has_recording = last_recording && last_recording_samples > 0,
        .seconds = last_recording_seconds,
        .samples = last_recording_samples,
        .level = last_level,
        .peak = last_peak,
        .channel = last_channel,
        .last_error = last_record_error,
    };
}

bool jr_mic_status(char *response, size_t capacity) {
    if (!response || capacity < 192) return false;
    jr_mic_recording_info_t info;
    jr_mic_get_recording_info(&info);
    snprintf(response, capacity,
             "JR_OK mic_status model=%s sck=%d ws=%d sd=%d recording=%d has_recording=%d seconds=%u samples=%u level=%d peak=%d channel=%c last=%s",
             JR_MIC_MODEL,JR_MIC_SCK_GPIO,JR_MIC_WS_GPIO,JR_MIC_SD_GPIO,
             info.recording?1:0,info.has_recording?1:0,info.seconds,(unsigned)info.samples,
             info.level,info.peak,info.channel?info.channel:'L',
             info.last_error==ESP_ERR_INVALID_STATE?"not_run":esp_err_to_name(info.last_error));
    return true;
}

static int16_t *allocate_recording(size_t samples) {
    size_t bytes = samples * sizeof(int16_t);
    if (last_recording && last_recording_capacity >= samples) return last_recording;

    int16_t *fresh = (int16_t *)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!fresh) fresh = (int16_t *)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
    if (!fresh) return NULL;

    if (last_recording) heap_caps_free(last_recording);
    last_recording = fresh;
    last_recording_capacity = samples;
    return last_recording;
}


static bool live_stream_claim(void) {
    bool ok = false;
    portENTER_CRITICAL(&live_stream_lock);
    if (!live_stream_active) {
        live_stream_active = true;
        live_stream_count++;
        ok = true;
    }
    portEXIT_CRITICAL(&live_stream_lock);
    return ok;
}

static void live_stream_release(void) {
    portENTER_CRITICAL(&live_stream_lock);
    live_stream_active = false;
    portEXIT_CRITICAL(&live_stream_lock);
}

static void live_stream_note_chunk(size_t bytes, uint32_t elapsed_ms) {
    portENTER_CRITICAL(&live_stream_lock);
    live_stream_chunks++;
    live_stream_bytes += bytes;
    live_stream_last_chunk_ms = elapsed_ms;
    if (elapsed_ms > live_stream_max_chunk_ms) live_stream_max_chunk_ms = elapsed_ms;
    portEXIT_CRITICAL(&live_stream_lock);
}

static void live_stream_note_error(void) {
    portENTER_CRITICAL(&live_stream_lock);
    live_stream_errors++;
    portEXIT_CRITICAL(&live_stream_lock);
}

static void jr_mic_stream_task(void *arg) {
    jr_mic_stream_ctx_t *ctx = (jr_mic_stream_ctx_t *)arg;
    httpd_req_t *req = ctx ? ctx->req : NULL;
    bool bus_owned = false;
    i2s_chan_handle_t rx = NULL;
    esp_err_t err = ESP_FAIL;

    if (!req) goto finish;

    if (!jr_audio_bus_acquire(1500)) {
        live_stream_note_error();
        httpd_resp_set_status(req, "409 Conflict");
        httpd_resp_set_type(req, "text/plain; charset=utf-8");
        (void)httpd_resp_sendstr(req, "JR_MIC_STREAM_ERROR audio_bus_busy");
        goto finish;
    }
    bus_owned = true;

    jr_audio_stop();
    err = mic_rx_open(&rx);
    if (err != ESP_OK) {
        live_stream_note_error();
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "text/plain; charset=utf-8");
        (void)httpd_resp_sendstr(req, "JR_MIC_STREAM_ERROR i2s_open");
        goto finish;
    }

    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "X-JrBot-Audio-Format", "pcm_s16le");
    httpd_resp_set_hdr(req, "X-JrBot-Sample-Rate", "16000");
    httpd_resp_set_hdr(req, "X-JrBot-Channels", "1");
    httpd_resp_set_hdr(req, "X-JrBot-Stream-Chunk-Ms", "80");
    httpd_resp_set_hdr(req, "X-JrBot-Live-Session", ctx->session);

    ESP_LOGI(TAG, "LIVE_STREAM_START session=%s chunk_ms=%d", ctx->session, JR_MIC_STREAM_CHUNK_MS);

    int32_t raw[JR_MIC_FRAMES];
    int16_t pcm[JR_MIC_STREAM_SAMPLES];

    while (true) {
        int64_t chunk_started = esp_timer_get_time();
        size_t written = 0;

        while (written < JR_MIC_STREAM_SAMPLES) {
            size_t bytes_read = 0;
            err = i2s_channel_read(rx, raw, sizeof(raw), &bytes_read, pdMS_TO_TICKS(200));
            if (err != ESP_OK) break;

            size_t frames = bytes_read / sizeof(int32_t);
            size_t take = JR_MIC_STREAM_SAMPLES - written;
            if (take > frames) take = frames;

            for (size_t i = 0; i < take; ++i) {
                int32_t v = raw[i] >> 14;
                if (v > 32767) v = 32767;
                if (v < -32768) v = -32768;
                pcm[written + i] = (int16_t)v;
            }
            written += take;
        }

        if (err != ESP_OK || written == 0) {
            live_stream_note_error();
            ESP_LOGW(TAG, "LIVE_STREAM_CAPTURE_ERROR session=%s error=%s samples=%u",
                     ctx->session, esp_err_to_name(err), (unsigned)written);
            break;
        }

        err = httpd_resp_send_chunk(req, (const char *)pcm, written * sizeof(int16_t));
        uint32_t elapsed_ms = (uint32_t)((esp_timer_get_time() - chunk_started + 500) / 1000);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "LIVE_STREAM_CLIENT_END session=%s send=%s",
                     ctx->session, esp_err_to_name(err));
            break;
        }

        live_stream_note_chunk(written * sizeof(int16_t), elapsed_ms);

        uint32_t chunks_snapshot;
        portENTER_CRITICAL(&live_stream_lock);
        chunks_snapshot = live_stream_chunks;
        portEXIT_CRITICAL(&live_stream_lock);
        if ((chunks_snapshot % 50u) == 0u) {
            ESP_LOGI(TAG, "LIVE_STREAM session=%s chunks=%lu last_ms=%lu bytes=%u",
                     ctx->session, (unsigned long)chunks_snapshot,
                     (unsigned long)elapsed_ms, (unsigned)(written * sizeof(int16_t)));
        }
    }

finish:
    if (rx) mic_rx_close(rx);
    if (bus_owned) jr_audio_bus_release();
    if (req) {
        (void)httpd_resp_send_chunk(req, NULL, 0);
        (void)httpd_req_async_handler_complete(req);
    }
    if (ctx) free(ctx);
    live_stream_release();
    vTaskDelete(NULL);
}

esp_err_t jr_mic_stream_handler(httpd_req_t *req) {
    if (!req) return ESP_ERR_INVALID_ARG;
    if (!live_stream_claim()) {
        httpd_resp_set_status(req, "409 Conflict");
        httpd_resp_set_type(req, "text/plain; charset=utf-8");
        return httpd_resp_sendstr(req, "JR_MIC_STREAM_ERROR already_active");
    }

    jr_mic_stream_ctx_t *ctx = (jr_mic_stream_ctx_t *)calloc(1, sizeof(*ctx));
    if (!ctx) {
        live_stream_release();
        httpd_resp_set_status(req, "500 Internal Server Error");
        return httpd_resp_sendstr(req, "JR_MIC_STREAM_ERROR no_memory");
    }

    strncpy(ctx->session, live_session, sizeof(ctx->session) - 1);
    char query[128] = {0};
    char session[32] = {0};
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK &&
        httpd_query_key_value(query, "session", session, sizeof(session)) == ESP_OK &&
        session[0]) {
        strncpy(ctx->session, session, sizeof(ctx->session) - 1);
    }

    esp_err_t err = httpd_req_async_handler_begin(req, &ctx->req);
    if (err != ESP_OK) {
        free(ctx);
        live_stream_release();
        return err;
    }

    BaseType_t created = xTaskCreate(jr_mic_stream_task, "jr_mic_stream",
                                     JR_MIC_STREAM_TASK_STACK, ctx, 5, NULL);
    if (created != pdPASS) {
        (void)httpd_req_async_handler_complete(ctx->req);
        free(ctx);
        live_stream_release();
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t jr_mic_live_pcm_handler(httpd_req_t *req) {
    int milliseconds = 200;
    char query[80] = {0};
    char value[16] = {0};
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK &&
        httpd_query_key_value(query, "ms", value, sizeof(value)) == ESP_OK) {
        milliseconds = atoi(value);
    }
    if (milliseconds < 80) milliseconds = 80;
    if (milliseconds > 500) milliseconds = 500;

    uint32_t seq = ++live_request_count;
    live_last_block_ms = (uint32_t)milliseconds;
    int64_t started_us = esp_timer_get_time();

    if (!jr_audio_bus_acquire(1500)) {
        live_busy_count++;
        live_error_count++;
        httpd_resp_set_status(req, "409 Conflict");
        httpd_resp_set_type(req, "text/plain; charset=utf-8");
        return httpd_resp_sendstr(req, "JR_MIC_LIVE_ERROR audio_bus_busy");
    }

    jr_audio_stop();
    i2s_chan_handle_t rx = NULL;
    esp_err_t err = mic_rx_open(&rx);
    if (err != ESP_OK) {
        live_error_count++;
        mic_rx_close(rx);
        jr_audio_bus_release();
        httpd_resp_set_status(req, "500 Internal Server Error");
        return httpd_resp_sendstr(req, "JR_MIC_LIVE_ERROR i2s_open");
    }

    const size_t target = ((size_t)JR_MIC_SAMPLE_RATE * (size_t)milliseconds) / 1000u;
    int16_t *pcm = (int16_t *)heap_caps_malloc(target * sizeof(int16_t), MALLOC_CAP_8BIT);
    if (!pcm) {
        live_error_count++;
        mic_rx_close(rx);
        jr_audio_bus_release();
        httpd_resp_set_status(req, "500 Internal Server Error");
        return httpd_resp_sendstr(req, "JR_MIC_LIVE_ERROR no_memory");
    }

    int32_t raw[JR_MIC_FRAMES];
    size_t written = 0;
    while (written < target) {
        size_t bytes_read = 0;
        err = i2s_channel_read(rx, raw, sizeof(raw), &bytes_read, pdMS_TO_TICKS(250));
        if (err != ESP_OK) break;
        size_t frames = bytes_read / sizeof(int32_t);
        size_t take = target - written;
        if (take > frames) take = frames;
        for (size_t i = 0; i < take; ++i) {
            int32_t v = raw[i] >> 14;
            if (v > 32767) v = 32767;
            if (v < -32768) v = -32768;
            pcm[written + i] = (int16_t)v;
        }
        written += take;
    }

    mic_rx_close(rx);
    jr_audio_bus_release();

    uint32_t capture_ms = (uint32_t)((esp_timer_get_time() - started_us + 500) / 1000);
    live_last_capture_ms = capture_ms;
    if (capture_ms > live_max_capture_ms) live_max_capture_ms = capture_ms;

    if (err != ESP_OK || written == 0) {
        live_error_count++;
        heap_caps_free(pcm);
        httpd_resp_set_status(req, "500 Internal Server Error");
        ESP_LOGW(TAG, "LIVE_PCM seq=%lu capture_ms=%lu error=%s samples=%u",
                 (unsigned long)seq, (unsigned long)capture_ms, esp_err_to_name(err), (unsigned)written);
        return httpd_resp_sendstr(req, "JR_MIC_LIVE_ERROR capture");
    }

    char samples[24], seq_text[24], capture_text[24];
    snprintf(samples, sizeof(samples), "%u", (unsigned)written);
    snprintf(seq_text, sizeof(seq_text), "%lu", (unsigned long)seq);
    snprintf(capture_text, sizeof(capture_text), "%lu", (unsigned long)capture_ms);
    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "X-JrBot-Audio-Format", "pcm_s16le");
    httpd_resp_set_hdr(req, "X-JrBot-Sample-Rate", "16000");
    httpd_resp_set_hdr(req, "X-JrBot-Channels", "1");
    httpd_resp_set_hdr(req, "X-JrBot-Samples", samples);
    httpd_resp_set_hdr(req, "X-JrBot-Live-Seq", seq_text);
    httpd_resp_set_hdr(req, "X-JrBot-Capture-Ms", capture_text);
    err = httpd_resp_send(req, (const char *)pcm, written * sizeof(int16_t));
    if (err == ESP_OK) {
        live_ok_count++;
        live_bytes_sent += written * sizeof(int16_t);
        if ((seq % 20u) == 0u) {
            ESP_LOGI(TAG, "LIVE_PCM seq=%lu ok=%lu errors=%lu busy=%lu capture_ms=%lu bytes_total=%llu",
                     (unsigned long)seq, (unsigned long)live_ok_count, (unsigned long)live_error_count,
                     (unsigned long)live_busy_count, (unsigned long)capture_ms,
                     (unsigned long long)live_bytes_sent);
        }
    } else {
        live_error_count++;
    }
    heap_caps_free(pcm);
    return err;
}

esp_err_t jr_mic_live_diag_handler(httpd_req_t *req) {
    char query[160] = {0};
    char reset[8] = {0};
    char session[32] = {0};
    bool do_reset = false;

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "reset", reset, sizeof(reset)) == ESP_OK) {
            do_reset = !strcmp(reset, "1") || !strcmp(reset, "true");
        }
        (void)httpd_query_key_value(query, "session", session, sizeof(session));
    }

    if (do_reset) {
        portENTER_CRITICAL(&live_stream_lock);
        live_request_count = 0;
        live_ok_count = 0;
        live_error_count = 0;
        live_busy_count = 0;
        live_bytes_sent = 0;
        live_last_capture_ms = 0;
        live_max_capture_ms = 0;
        live_last_block_ms = 0;
        live_stream_count = 0;
        live_stream_chunks = 0;
        live_stream_errors = 0;
        live_stream_bytes = 0;
        live_stream_last_chunk_ms = 0;
        live_stream_max_chunk_ms = 0;
        if (session[0]) {
            strncpy(live_session, session, sizeof(live_session) - 1);
            live_session[sizeof(live_session) - 1] = 0;
        } else {
            strcpy(live_session, "none");
        }
        portEXIT_CRITICAL(&live_stream_lock);
        ESP_LOGI(TAG, "LIVE_DIAG_RESET session=%s", live_session);
    }

    uint32_t request_count, ok_count, error_count, busy_count;
    uint64_t legacy_bytes, stream_bytes;
    uint32_t last_capture, max_capture, last_block;
    uint32_t stream_count, stream_chunks, stream_errors, stream_last_ms, stream_max_ms;
    bool stream_active;
    char session_copy[32];

    portENTER_CRITICAL(&live_stream_lock);
    request_count = live_request_count;
    ok_count = live_ok_count;
    error_count = live_error_count;
    busy_count = live_busy_count;
    legacy_bytes = live_bytes_sent;
    last_capture = live_last_capture_ms;
    max_capture = live_max_capture_ms;
    last_block = live_last_block_ms;
    stream_count = live_stream_count;
    stream_chunks = live_stream_chunks;
    stream_errors = live_stream_errors;
    stream_bytes = live_stream_bytes;
    stream_last_ms = live_stream_last_chunk_ms;
    stream_max_ms = live_stream_max_chunk_ms;
    stream_active = live_stream_active;
    strncpy(session_copy, live_session, sizeof(session_copy) - 1);
    session_copy[sizeof(session_copy) - 1] = 0;
    portEXIT_CRITICAL(&live_stream_lock);

    char body[768];
    snprintf(body, sizeof(body),
             "JR_LIVE_DIAG session=%s mode=stream stream_active=%d streams=%lu chunks=%lu stream_errors=%lu stream_bytes=%llu last_chunk_ms=%lu max_chunk_ms=%lu chunk_ms=%d sample_rate=%d channels=1 format=pcm_s16le legacy_requests=%lu legacy_ok=%lu legacy_errors=%lu legacy_busy=%lu legacy_bytes=%llu legacy_last_capture_ms=%lu legacy_max_capture_ms=%lu legacy_last_block_ms=%lu",
             session_copy,
             stream_active ? 1 : 0,
             (unsigned long)stream_count,
             (unsigned long)stream_chunks,
             (unsigned long)stream_errors,
             (unsigned long long)stream_bytes,
             (unsigned long)stream_last_ms,
             (unsigned long)stream_max_ms,
             JR_MIC_STREAM_CHUNK_MS,
             JR_MIC_SAMPLE_RATE,
             (unsigned long)request_count,
             (unsigned long)ok_count,
             (unsigned long)error_count,
             (unsigned long)busy_count,
             (unsigned long long)legacy_bytes,
             (unsigned long)last_capture,
             (unsigned long)max_capture,
             (unsigned long)last_block);
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, body);
}

esp_err_t jr_mic_record_wav_handler(httpd_req_t *req) {
    int seconds = 3;
    char query[80] = {0};
    char value[16] = {0};
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK &&
        httpd_query_key_value(query, "seconds", value, sizeof(value)) == ESP_OK) {
        seconds = atoi(value);
    }
    if (seconds < 1) seconds = 1;
    if (seconds > JR_MIC_MAX_RECORD_SECONDS) seconds = JR_MIC_MAX_RECORD_SECONDS;

    if (!jr_audio_bus_acquire(JR_MIC_RECORD_BUS_TIMEOUT_MS)) {
        httpd_resp_set_status(req, "409 Conflict");
        httpd_resp_set_type(req, "text/plain; charset=utf-8");
        return httpd_resp_sendstr(req, "JR_MIC_ERROR audio_bus_busy");
    }

    recording_active = true;
    last_record_error = ESP_ERR_INVALID_STATE;
    last_recording_samples = 0;
    last_recording_seconds = 0;
    jr_audio_stop();

    const size_t target_samples = (size_t)JR_MIC_SAMPLE_RATE * (size_t)seconds;
    int16_t *recording = allocate_recording(target_samples);
    if (!recording) {
        recording_active = false;
        last_record_error = ESP_ERR_NO_MEM;
        jr_audio_bus_release();
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "text/plain; charset=utf-8");
        return httpd_resp_sendstr(req, "JR_MIC_ERROR sem_memoria_para_gravacao");
    }

    i2s_chan_handle_t rx = NULL;
    esp_err_t err = mic_rx_open(&rx);
    if (err != ESP_OK) goto record_done;

    int32_t raw[JR_MIC_FRAMES];
    size_t written = 0;
    int64_t sum_abs = 0;
    int peak = 0;
    unsigned counted = 0;

    while (written < target_samples) {
        size_t bytes_read = 0;
        err = i2s_channel_read(rx, raw, sizeof(raw), &bytes_read, pdMS_TO_TICKS(300));
        if (err != ESP_OK) break;
        unsigned frames = (unsigned)(bytes_read / sizeof(int32_t));
        if (!frames) continue;
        last_channel = 'L';
        size_t take = target_samples - written;
        if (take > frames) take = frames;
        for (size_t i = 0; i < take; ++i) {
            int32_t v = raw[i] >> 14;
            if (v > 32767) v = 32767;
            if (v < -32768) v = -32768;
            int16_t sample = (int16_t)v;
            recording[written + i] = sample;
            int a = abs16(sample);
            sum_abs += a;
            if (a > peak) peak = a;
            counted++;
        }
        written += take;
    }

    last_recording_samples = written;
    last_recording_seconds = (unsigned)(written / JR_MIC_SAMPLE_RATE);
    if (written && last_recording_seconds == 0) last_recording_seconds = 1;
    last_level = counted ? (int)(sum_abs / counted) : 0;
    last_peak = peak;
    if (err == ESP_OK && written < target_samples) err = ESP_ERR_TIMEOUT;

record_done:
    mic_rx_close(rx);
    recording_active = false;
    last_record_error = err;
    jr_audio_bus_release();

    if (err != ESP_OK || last_recording_samples == 0) {
        char message[128];
        snprintf(message,sizeof(message),"JR_MIC_ERROR record=%s samples=%u",
                 esp_err_to_name(err),(unsigned)last_recording_samples);
        ESP_LOGE(TAG,"%s",message);
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_set_type(req, "text/plain; charset=utf-8");
        return httpd_resp_sendstr(req, message);
    }

    const uint32_t data_bytes = (uint32_t)(last_recording_samples * sizeof(int16_t));
    uint8_t header[44] = {0};
    make_wav_header(header, JR_MIC_SAMPLE_RATE, data_bytes);
    httpd_resp_set_type(req, "audio/wav");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=jrbot-microfone.wav");
    err = httpd_resp_send_chunk(req, (const char *)header, sizeof(header));
    if (err == ESP_OK) {
        const uint8_t *bytes = (const uint8_t *)last_recording;
        size_t sent = 0;
        while (sent < data_bytes) {
            size_t chunk = data_bytes - sent;
            if (chunk > 4096) chunk = 4096;
            err = httpd_resp_send_chunk(req, (const char *)(bytes + sent), chunk);
            if (err != ESP_OK) break;
            sent += chunk;
        }
    }
    (void)httpd_resp_send_chunk(req, NULL, 0);
    if (err == ESP_OK) {
        ESP_LOGI(TAG,"RECORDED seconds=%d samples=%u level=%d peak=%d channel=%c",
                 seconds,(unsigned)last_recording_samples,last_level,last_peak,last_channel);
    }
    return err;
}

esp_err_t jr_mic_play_last_recording(void) {
    if (!last_recording || last_recording_samples == 0 || recording_active) return ESP_ERR_INVALID_STATE;
    ESP_LOGI(TAG,"PLAY_LAST samples=%u seconds=%u channel=%c",
             (unsigned)last_recording_samples,last_recording_seconds,last_channel);
    return jr_audio_play_pcm16_mono(last_recording,last_recording_samples,JR_MIC_SAMPLE_RATE);
}
