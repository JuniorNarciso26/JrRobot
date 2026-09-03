#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "driver/i2s_std.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "jr_audio.h"

#define JR_AUDIO_BCLK_GPIO 15
#define JR_AUDIO_LRC_GPIO 16
#define JR_AUDIO_DIN_GPIO 17
#define JR_AUDIO_SAMPLE_RATE 16000
#define JR_AUDIO_PI 3.14159265358979323846f

static const char *TAG = "jrbot_audio";
static i2s_chan_handle_t tx_chan = NULL;
static bool audio_ready = false;
static int audio_volume = 35;
static char status_text[96] = "nao iniciado";

esp_err_t jr_audio_start(void) {
    if (audio_ready) return ESP_OK;

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
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    err = i2s_channel_init_std_mode(tx_chan, &std_cfg);
    if (err != ESP_OK) {
        snprintf(status_text, sizeof(status_text), "erro init std=%s", esp_err_to_name(err));
        ESP_LOGE(TAG, "%s", status_text);
        return err;
    }

    err = i2s_channel_enable(tx_chan);
    if (err != ESP_OK) {
        snprintf(status_text, sizeof(status_text), "erro enable=%s", esp_err_to_name(err));
        ESP_LOGE(TAG, "%s", status_text);
        return err;
    }

    audio_ready = true;
    snprintf(status_text, sizeof(status_text), "ok MAX98357A bclk=%d lrc=%d din=%d volume=%d", JR_AUDIO_BCLK_GPIO, JR_AUDIO_LRC_GPIO, JR_AUDIO_DIN_GPIO, audio_volume);
    ESP_LOGI(TAG, "%s", status_text);
    return ESP_OK;
}

bool jr_audio_ready(void) {
    return audio_ready;
}

int jr_audio_volume(void) {
    return audio_volume;
}

void jr_audio_set_volume(int volume) {
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    audio_volume = volume;
    snprintf(status_text, sizeof(status_text), "ok MAX98357A bclk=%d lrc=%d din=%d volume=%d", JR_AUDIO_BCLK_GPIO, JR_AUDIO_LRC_GPIO, JR_AUDIO_DIN_GPIO, audio_volume);
    ESP_LOGI(TAG, "volume=%d", audio_volume);
}

esp_err_t jr_audio_test_tone(int frequency_hz, int duration_ms) {
    if (!audio_ready) {
        esp_err_t err = jr_audio_start();
        if (err != ESP_OK) return err;
    }
    if (frequency_hz < 100) frequency_hz = 100;
    if (frequency_hz > 3000) frequency_hz = 3000;
    if (duration_ms < 100) duration_ms = 100;
    if (duration_ms > 5000) duration_ms = 5000;

    const int frames_per_chunk = 256;
    int16_t samples[frames_per_chunk * 2];
    int total_frames = (JR_AUDIO_SAMPLE_RATE * duration_ms) / 1000;
    int written_frames = 0;
    float phase = 0.0f;
    float step = 2.0f * JR_AUDIO_PI * (float)frequency_hz / (float)JR_AUDIO_SAMPLE_RATE;
    int amplitude = (audio_volume * 26000) / 100;
    if (amplitude < 0) amplitude = 0;
    if (amplitude > 26000) amplitude = 26000;

    ESP_LOGI(TAG, "teste audio freq=%d dur=%d volume=%d", frequency_hz, duration_ms, audio_volume);
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
        esp_err_t err = i2s_channel_write(tx_chan, samples, (size_t)frames * 2 * sizeof(int16_t), &bytes_written, pdMS_TO_TICKS(1000));
        if (err != ESP_OK) return err;
        written_frames += frames;
    }

    memset(samples, 0, sizeof(samples));
    size_t bytes_written = 0;
    i2s_channel_write(tx_chan, samples, sizeof(samples), &bytes_written, pdMS_TO_TICKS(200));
    return ESP_OK;
}

const char *jr_audio_status_text(void) {
    return status_text;
}
