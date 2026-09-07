#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "jr_audio.h"
#include "jr_board.h"

#define JR_AUDIO_SAMPLE_RATE 16000
#define JR_AUDIO_PI 3.14159265358979323846f

static portMUX_TYPE audio_state_lock=portMUX_INITIALIZER_UNLOCKED;
static const char *TAG = "jrbot_audio";
static i2s_chan_handle_t tx_chan = NULL;
static SemaphoreHandle_t i2s_bus_mutex = NULL;
static bool audio_ready = false;
static int audio_volume = 35;
static jr_audio_diag_t audio_diag = {.running=false,.tests=0,.last_bytes=0,.last_frequency_hz=0,.last_duration_ms=0,.last_error=ESP_ERR_INVALID_STATE};
static char status_text[192] = "MAX98357A pronto para teste sob demanda";

esp_err_t jr_audio_bus_init(void) {
    if (!i2s_bus_mutex) i2s_bus_mutex = xSemaphoreCreateMutex();
    return i2s_bus_mutex ? ESP_OK : ESP_ERR_NO_MEM;
}

bool jr_audio_bus_acquire(uint32_t timeout_ms) {
    if (jr_audio_bus_init() != ESP_OK) return false;
    return xSemaphoreTake(i2s_bus_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void jr_audio_bus_release(void) {
    if (i2s_bus_mutex) xSemaphoreGive(i2s_bus_mutex);
}

static void set_ready(bool ready) {
    portENTER_CRITICAL(&audio_state_lock);
    audio_ready = ready;
    portEXIT_CRITICAL(&audio_state_lock);
}

static uint32_t audio_diag_begin(int frequency_hz, int duration_ms) {
    uint32_t seq;
    portENTER_CRITICAL(&audio_state_lock);
    audio_diag.tests++;
    audio_diag.running = true;
    audio_diag.last_bytes = 0;
    audio_diag.last_frequency_hz = frequency_hz;
    audio_diag.last_duration_ms = duration_ms;
    audio_diag.last_error = ESP_ERR_INVALID_STATE;
    seq = audio_diag.tests;
    portEXIT_CRITICAL(&audio_state_lock);
    return seq;
}

static void audio_diag_finish(uint32_t bytes, esp_err_t err) {
    portENTER_CRITICAL(&audio_state_lock);
    audio_diag.running = false;
    audio_diag.last_bytes = bytes;
    audio_diag.last_error = err;
    portEXIT_CRITICAL(&audio_state_lock);
}

void jr_audio_get_diag(jr_audio_diag_t *out) {
    if (!out) return;
    portENTER_CRITICAL(&audio_state_lock);
    *out = audio_diag;
    portEXIT_CRITICAL(&audio_state_lock);
}

static void release_shared_pins(void) {
    (void)gpio_reset_pin((gpio_num_t)JR_AUDIO_BCLK_GPIO);
    (void)gpio_set_pull_mode((gpio_num_t)JR_AUDIO_BCLK_GPIO, GPIO_FLOATING);
    (void)gpio_set_direction((gpio_num_t)JR_AUDIO_BCLK_GPIO, GPIO_MODE_INPUT);
    (void)gpio_reset_pin((gpio_num_t)JR_AUDIO_LRC_GPIO);
    (void)gpio_set_pull_mode((gpio_num_t)JR_AUDIO_LRC_GPIO, GPIO_FLOATING);
    (void)gpio_set_direction((gpio_num_t)JR_AUDIO_LRC_GPIO, GPIO_MODE_INPUT);
    (void)gpio_set_direction((gpio_num_t)JR_AUDIO_DIN_GPIO, GPIO_MODE_OUTPUT);
    (void)gpio_set_level((gpio_num_t)JR_AUDIO_DIN_GPIO, 0);
}

void jr_audio_stop(void) {
    if (tx_chan) {
        if (jr_audio_ready()) (void)i2s_channel_disable(tx_chan);
        (void)i2s_del_channel(tx_chan);
        tx_chan = NULL;
    }
    set_ready(false);
    release_shared_pins();
}

esp_err_t jr_audio_start(void) {
    if (!JR_AUDIO_ENABLED) return ESP_ERR_NOT_SUPPORTED;
    if (jr_audio_ready()) return ESP_OK;
    if (tx_chan) jr_audio_stop();

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    esp_err_t err = i2s_new_channel(&chan_cfg, &tx_chan, NULL);
    if (err != ESP_OK) {
        snprintf(status_text, sizeof(status_text), "erro i2s_new_channel=%s", esp_err_to_name(err));
        ESP_LOGE(TAG, "%s", status_text);
        return err;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(JR_AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = JR_AUDIO_BCLK_GPIO,
            .ws = JR_AUDIO_LRC_GPIO,
            .dout = JR_AUDIO_DIN_GPIO,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {.mclk_inv=false,.bclk_inv=false,.ws_inv=false},
        },
    };

    err = i2s_channel_init_std_mode(tx_chan, &std_cfg);
    if (err != ESP_OK) {
        snprintf(status_text, sizeof(status_text), "erro init std=%s", esp_err_to_name(err));
        ESP_LOGE(TAG, "%s", status_text);
        jr_audio_stop();
        return err;
    }
    err = i2s_channel_enable(tx_chan);
    if (err != ESP_OK) {
        snprintf(status_text, sizeof(status_text), "erro enable=%s", esp_err_to_name(err));
        ESP_LOGE(TAG, "%s", status_text);
        jr_audio_stop();
        return err;
    }
    set_ready(true);
    snprintf(status_text, sizeof(status_text), "ok MAX98357A bclk=%d lrc=%d din=%d rate=%d volume=%d",
             JR_AUDIO_BCLK_GPIO,JR_AUDIO_LRC_GPIO,JR_AUDIO_DIN_GPIO,JR_AUDIO_SAMPLE_RATE,audio_volume);
    ESP_LOGI(TAG, "%s", status_text);
    return ESP_OK;
}

bool jr_audio_ready(void) {
    portENTER_CRITICAL(&audio_state_lock);
    bool ready=audio_ready;
    portEXIT_CRITICAL(&audio_state_lock);
    return ready;
}

int jr_audio_volume(void) {
    portENTER_CRITICAL(&audio_state_lock);
    int value=audio_volume;
    portEXIT_CRITICAL(&audio_state_lock);
    return value;
}

void jr_audio_set_volume(int volume) {
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    portENTER_CRITICAL(&audio_state_lock);
    audio_volume = volume;
    portEXIT_CRITICAL(&audio_state_lock);
    snprintf(status_text, sizeof(status_text), "ok MAX98357A bclk=%d lrc=%d din=%d volume=%d",
             JR_AUDIO_BCLK_GPIO,JR_AUDIO_LRC_GPIO,JR_AUDIO_DIN_GPIO,audio_volume);
}

esp_err_t jr_audio_test_tone(int frequency_hz, int duration_ms) {
    if (frequency_hz < 100) frequency_hz = 100;
    if (frequency_hz > 3000) frequency_hz = 3000;
    if (duration_ms < 100) duration_ms = 100;
    if (duration_ms > 5000) duration_ms = 5000;
    if (!jr_audio_bus_acquire(15000)) return ESP_ERR_TIMEOUT;

    uint32_t seq = audio_diag_begin(frequency_hz, duration_ms);
    uint32_t tone_bytes = 0;
    esp_err_t err = ESP_OK;
    enum { frames_per_chunk = 256 };
    int16_t samples[frames_per_chunk * 2];
    int total_frames = (JR_AUDIO_SAMPLE_RATE * duration_ms) / 1000;
    int written_frames = 0;
    float phase = 0.0f;
    float step = 2.0f * JR_AUDIO_PI * (float)frequency_hz / (float)JR_AUDIO_SAMPLE_RATE;
    int amplitude = (jr_audio_volume() * 26000) / 100;
    if (amplitude < 0) amplitude = 0;
    if (amplitude > 26000) amplitude = 26000;

    ESP_LOGI(TAG, "TEST_BEGIN seq=%lu freq=%d dur=%d volume=%d bclk=%d lrc=%d din=%d rate=%d format=MSB",
             (unsigned long)seq,frequency_hz,duration_ms,jr_audio_volume(),
             JR_AUDIO_BCLK_GPIO,JR_AUDIO_LRC_GPIO,JR_AUDIO_DIN_GPIO,JR_AUDIO_SAMPLE_RATE);

    err = jr_audio_start();
    if (err != ESP_OK) goto done;

    while (written_frames < total_frames) {
        int frames = total_frames - written_frames;
        if (frames > frames_per_chunk) frames = frames_per_chunk;
        for (int i = 0; i < frames; ++i) {
            int16_t sample = (int16_t)(sinf(phase) * amplitude);
            samples[i * 2] = sample;
            samples[i * 2 + 1] = sample;
            phase += step;
            if (phase > 2.0f * JR_AUDIO_PI) phase -= 2.0f * JR_AUDIO_PI;
        }
        size_t bytes_written = 0;
        size_t requested = (size_t)frames * 2 * sizeof(int16_t);
        err = i2s_channel_write(tx_chan, samples, requested, &bytes_written, pdMS_TO_TICKS(1000));
        if (err != ESP_OK || bytes_written != requested) {
            if (err == ESP_OK) err = ESP_ERR_INVALID_SIZE;
            goto done;
        }
        tone_bytes += (uint32_t)bytes_written;
        written_frames += frames;
    }

    memset(samples, 0, sizeof(samples));
    {
        size_t bytes_written = 0;
        esp_err_t silence_err = i2s_channel_write(tx_chan, samples, sizeof(samples), &bytes_written, pdMS_TO_TICKS(200));
        if (silence_err != ESP_OK) err = silence_err;
    }

done:
    jr_audio_stop();
    audio_diag_finish(tone_bytes, err);
    if (err == ESP_OK) {
        snprintf(status_text, sizeof(status_text), "MAX98357A teste seq=%lu transmitido bytes=%lu; I2S liberado para o microfone",
                 (unsigned long)seq,(unsigned long)tone_bytes);
        ESP_LOGI(TAG, "TEST_END seq=%lu result=ESP_OK tone_bytes=%lu i2s_released=1",
                 (unsigned long)seq,(unsigned long)tone_bytes);
    } else {
        snprintf(status_text, sizeof(status_text), "MAX98357A teste seq=%lu erro=%s bytes=%lu",
                 (unsigned long)seq,esp_err_to_name(err),(unsigned long)tone_bytes);
        ESP_LOGE(TAG, "TEST_END seq=%lu result=%s tone_bytes=%lu",
                 (unsigned long)seq,esp_err_to_name(err),(unsigned long)tone_bytes);
    }
    jr_audio_bus_release();
    return err;
}

esp_err_t jr_audio_play_pcm16_mono(const int16_t *input, size_t sample_count, int sample_rate_hz) {
    if (!input || sample_count == 0) return ESP_ERR_INVALID_ARG;
    if (sample_rate_hz != JR_AUDIO_SAMPLE_RATE) return ESP_ERR_NOT_SUPPORTED;
    if (!jr_audio_bus_acquire(15000)) return ESP_ERR_TIMEOUT;

    esp_err_t err = jr_audio_start();
    if (err != ESP_OK) {
        jr_audio_bus_release();
        return err;
    }

    enum { frames_per_chunk = 256 };
    int16_t stereo[frames_per_chunk * 2];
    size_t pos = 0;
    uint32_t bytes_total = 0;
    int gain = jr_audio_volume();

    ESP_LOGI(TAG, "PLAY_RECORDING_BEGIN samples=%u rate=%d volume=%d",
             (unsigned)sample_count,sample_rate_hz,gain);

    while (pos < sample_count) {
        size_t frames = sample_count - pos;
        if (frames > frames_per_chunk) frames = frames_per_chunk;
        for (size_t i = 0; i < frames; ++i) {
            int32_t sample = ((int32_t)input[pos + i] * gain) / 100;
            if (sample > 32767) sample = 32767;
            if (sample < -32768) sample = -32768;
            stereo[i * 2] = (int16_t)sample;
            stereo[i * 2 + 1] = (int16_t)sample;
        }
        size_t bytes_written = 0;
        size_t requested = frames * 2 * sizeof(int16_t);
        err = i2s_channel_write(tx_chan, stereo, requested, &bytes_written, pdMS_TO_TICKS(1000));
        if (err != ESP_OK || bytes_written != requested) {
            if (err == ESP_OK) err = ESP_ERR_INVALID_SIZE;
            break;
        }
        bytes_total += (uint32_t)bytes_written;
        pos += frames;
    }

    memset(stereo, 0, sizeof(stereo));
    if (err == ESP_OK) {
        size_t ignored = 0;
        err = i2s_channel_write(tx_chan, stereo, sizeof(stereo), &ignored, pdMS_TO_TICKS(200));
    }
    jr_audio_stop();
    jr_audio_bus_release();

    if (err == ESP_OK) {
        snprintf(status_text, sizeof(status_text), "ultima gravacao reproduzida samples=%u bytes=%lu volume=%d",
                 (unsigned)sample_count,(unsigned long)bytes_total,gain);
        ESP_LOGI(TAG, "PLAY_RECORDING_END result=ESP_OK bytes=%lu",(unsigned long)bytes_total);
    } else {
        snprintf(status_text, sizeof(status_text), "erro reproduzindo gravacao=%s",esp_err_to_name(err));
        ESP_LOGE(TAG, "PLAY_RECORDING_END result=%s",esp_err_to_name(err));
    }
    return err;
}

const char *jr_audio_status_text(void) { return status_text; }
