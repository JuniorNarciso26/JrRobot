/*
 * JrBot HW04 audio-only hardware smoke test for the Ivy AI playground.
 * This is independent of the original Ivy application.
 * No camera, Wi-Fi, display, SD card, codec ES8311, STT, LLM or TTS.
 * SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include "esp_check.h"
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../../hw04_audio_config.h"

#define FRAMES 256
#define FREQ_HZ 440
#define TONE_MS 280

static const char *TAG = "ivy_hw04_audio";
static i2s_chan_handle_t tx_chan;
static i2s_chan_handle_t rx_chan;
/* Stereo interleaved 32-bit I2S frames. The mic is strapped LEFT (L/R=GND). */
static int32_t capture[FRAMES * 2];
static int32_t output[FRAMES * 2];

static esp_err_t start_audio(void)
{
    i2s_chan_config_t cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    cfg.dma_desc_num = 6;
    cfg.dma_frame_num = FRAMES;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&cfg, &tx_chan, &rx_chan), TAG, "create I2S");

    i2s_std_config_t standard = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(IVY_HW04_AUDIO_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)IVY_HW04_I2S_BCLK,
            .ws = (gpio_num_t)IVY_HW04_I2S_LRCK,
            .dout = (gpio_num_t)IVY_HW04_I2S_DOUT,
            .din = (gpio_num_t)IVY_HW04_I2S_DIN,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false }
        }
    };
    standard.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(tx_chan, &standard), TAG, "TX config");
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(rx_chan, &standard), TAG, "RX config");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(tx_chan), TAG, "TX enable");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(rx_chan), TAG, "RX enable");
    return ESP_OK;
}

/* Low-amplitude triangle tone; avoids floating point and includes both slots. */
static void play_tone(void)
{
    ESP_LOGI(TAG, "Playing short low-volume 440 Hz test tone");
    uint32_t phase = 0;
    const uint32_t step = (uint32_t)(((uint64_t)FREQ_HZ << 32) / IVY_HW04_AUDIO_RATE);
    int remaining = IVY_HW04_AUDIO_RATE * TONE_MS / 1000;
    while (remaining > 0) {
        int n = remaining > FRAMES ? FRAMES : remaining;
        for (int i = 0; i < n; ++i) {
            phase += step;
            uint32_t x = phase >> 16;
            int32_t triangle = x < 32768U ? (int32_t)x : (int32_t)(65535U - x);
            int32_t amplitude = ((triangle - 16384) * 128);
            output[2 * i] = amplitude;
            output[2 * i + 1] = amplitude;
        }
        size_t sent = 0;
        esp_err_t err = i2s_channel_write(tx_chan, output, n * 2 * sizeof(int32_t),
                                          &sent, pdMS_TO_TICKS(1000));
        if (err != ESP_OK || sent != (size_t)n * 2 * sizeof(int32_t)) {
            ESP_LOGW(TAG, "TX incomplete: %s sent=%u", esp_err_to_name(err), (unsigned)sent);
            break;
        }
        remaining -= n;
    }
    for (int i = 0; i < FRAMES * 2; ++i) output[i] = 0;
    size_t ignored = 0;
    i2s_channel_write(tx_chan, output, sizeof(output), &ignored, pdMS_TO_TICKS(1000));
}

void app_main(void)
{
    ESP_LOGI(TAG, "JrBot HW04 Ivy playground / audio-only / 16kHz / 32-bit I2S");
    ESP_LOGI(TAG, "GPIO: BCLK=%d WS=%d MIC=%d AMP=%d",
             IVY_HW04_I2S_BCLK, IVY_HW04_I2S_LRCK,
             IVY_HW04_I2S_DIN, IVY_HW04_I2S_DOUT);
    ESP_ERROR_CHECK(start_audio());
    play_tone();
    ESP_LOGI(TAG, "Microphone capture active; speak near JrBot.");
    while (true) {
        uint32_t peak = 0;
        uint64_t magnitude = 0;
        uint32_t frames = 0;
        for (int block = 0; block < 64; ++block) {
            size_t got = 0;
            esp_err_t err = i2s_channel_read(rx_chan, capture, sizeof(capture),
                                             &got, pdMS_TO_TICKS(500));
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "RX: %s", esp_err_to_name(err));
                continue;
            }
            uint32_t count = (uint32_t)(got / (2 * sizeof(int32_t)));
            for (uint32_t i = 0; i < count; i++) {
                int32_t sample = capture[i * 2] >> 16; /* LEFT 16 MSBs */
                uint32_t absval = (uint32_t)(sample < 0 ? -sample : sample);
                if (absval > peak) peak = absval;
                magnitude += absval;
                frames++;
            }
        }
        ESP_LOGI(TAG, "MIC frames=%" PRIu32 " peak=%" PRIu32
                 " avg_abs=%" PRIu32 " (expect variation with speech)",
                 frames, peak, frames ? (uint32_t)(magnitude / frames) : 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
