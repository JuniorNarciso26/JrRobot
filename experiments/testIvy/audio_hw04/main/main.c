/*
 * JrBot HW04 audio-only smoke test for Ivy integration.
 * Reference for playback: firmware/module_audio/jr_audio.c on main
 * ("jr_audio_test_tone": 16-bit stereo Philips I2S + sine samples).
 * No automatic speaker playback; commands over ESP-IDF console stdin:
 *   teste som
 *   iniciar ivy
 * SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "esp_check.h"
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../../hw04_audio_config.h"

#define FRAMES 256
#define AUDIO_PI 3.14159265358979323846f
static const char *TAG = "ivy_hw04_audio";
static i2s_chan_handle_t rx_chan = NULL;
static int32_t capture[FRAMES * 2];

static esp_err_t start_mic(void)
{
    i2s_chan_config_t cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    cfg.dma_desc_num = 6;
    cfg.dma_frame_num = FRAMES;
    esp_err_t err = i2s_new_channel(&cfg, NULL, &rx_chan);
    if (err != ESP_OK) return err;
    i2s_std_config_t standard = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(IVY_HW04_AUDIO_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)IVY_HW04_I2S_BCLK,
            .ws = (gpio_num_t)IVY_HW04_I2S_LRCK,
            .dout = I2S_GPIO_UNUSED,
            .din = (gpio_num_t)IVY_HW04_I2S_DIN
        }
    };
    standard.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;
    err = i2s_channel_init_std_mode(rx_chan, &standard);
    if (err == ESP_OK) err = i2s_channel_enable(rx_chan);
    if (err != ESP_OK) {
        i2s_del_channel(rx_chan);
        rx_chan = NULL;
    }
    return err;
}

static void stop_mic(void)
{
    if (rx_chan) {
        i2s_channel_disable(rx_chan);
        i2s_del_channel(rx_chan);
        rx_chan = NULL;
    }
}

/* Reuses the tested original JrBot's sine-wave, stereo 16-bit PCM behavior.
 * Audio ownership is exclusive: RX stopped, TX played, RX restarted. */
static esp_err_t play_speaker_test(void)
{
    stop_mic();
    i2s_chan_handle_t tx = NULL;
    i2s_chan_config_t cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    cfg.dma_desc_num = 6;
    cfg.dma_frame_num = FRAMES;
    esp_err_t err = i2s_new_channel(&cfg, &tx, NULL);
    if (err != ESP_OK) goto restore_mic;
    i2s_std_config_t standard = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(IVY_HW04_AUDIO_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)IVY_HW04_I2S_BCLK,
            .ws = (gpio_num_t)IVY_HW04_I2S_LRCK,
            .dout = (gpio_num_t)IVY_HW04_I2S_DOUT,
            .din = I2S_GPIO_UNUSED
        }
    };
    standard.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;
    err = i2s_channel_init_std_mode(tx, &standard);
    if (err != ESP_OK) goto close_tx;
    err = i2s_channel_enable(tx);
    if (err != ESP_OK) goto close_tx;
    ESP_LOGI(TAG, "SPEAKER_TEST_START freq=440 duration_ms=1500 rate=16000 format=PHILIPS_16_STEREO");
    int16_t samples[FRAMES * 2];
    const int total = IVY_HW04_AUDIO_RATE * 1500 / 1000;
    float phase = 0.0f;
    const float step = 2.0f * AUDIO_PI * 440.0f / IVY_HW04_AUDIO_RATE;
    uint32_t sent_total = 0;
    for (int written = 0; written < total;) {
        int n = total - written;
        if (n > FRAMES) n = FRAMES;
        for (int i = 0; i < n; i++) {
            int16_t sample = (int16_t)(sinf(phase) * 7800.0f);
            samples[i * 2] = sample;
            samples[i * 2 + 1] = sample;
            phase += step;
            if (phase >= 2.0f * AUDIO_PI) phase -= 2.0f * AUDIO_PI;
        }
        size_t sent = 0;
        size_t expected = n * 2 * sizeof(int16_t);
        err = i2s_channel_write(tx, samples, expected, &sent, pdMS_TO_TICKS(1500));
        sent_total += sent;
        if (err != ESP_OK || sent != expected) {
            if (err == ESP_OK) err = ESP_ERR_INVALID_SIZE;
            break;
        }
        written += n;
    }
    memset(samples, 0, sizeof(samples));
    size_t ignored = 0;
    (void)i2s_channel_write(tx, samples, sizeof(samples), &ignored, pdMS_TO_TICKS(500));
    ESP_LOGI(TAG, "SPEAKER_TEST_END result=%s bytes=%" PRIu32, esp_err_to_name(err), sent_total);
    i2s_channel_disable(tx);
close_tx:
    i2s_del_channel(tx);
restore_mic:
    {
        esp_err_t mic_result = start_mic();
        if (mic_result != ESP_OK) {
            ESP_LOGE(TAG, "MIC_RESTART_FAIL %s", esp_err_to_name(mic_result));
            if (err == ESP_OK) err = mic_result;
        }
    }
    return err;
}

static void serial_task(void *arg)
{
    (void)arg;
    char line[80];
    size_t used = 0;
    ESP_LOGI(TAG, "Comandos Serial: teste som | iniciar ivy | ajuda");
    while (true) {
        int ch = getchar();  /* ESP-IDF configured console (UART or USB-JTAG) */
        if (ch == EOF) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        if (ch == '\r' || ch == '\n') {
            if (used == 0) continue;
            line[used] = 0;
            used = 0;
            ESP_LOGI(TAG, "SERIAL_COMMAND %s", line);
            if (strcmp(line, "teste som") == 0) {
                ESP_LOGI(TAG, "TESTE_SOM solicitado");
                esp_err_t result = play_speaker_test();
                ESP_LOGI(TAG, "TESTE_SOM resultado=%s", esp_err_to_name(result));
            } else if (strcmp(line, "iniciar ivy") == 0) {
                ESP_LOGW(TAG, "IVY_NAO_DISPONIVEL: reconhecimento, LLM e TTS ainda nao integrados neste firmware de diagnostico. Nenhuma IA foi iniciada.");
            } else if (strcmp(line, "ajuda") == 0) {
                ESP_LOGI(TAG, "Comandos: teste som | iniciar ivy | ajuda");
            } else {
                ESP_LOGW(TAG, "Comando desconhecido. Digite ajuda.");
            }
            continue;
        }
        if (used < sizeof(line) - 1) line[used++] = (char)ch;
        else used = 0;
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "JrBot HW04 testIvy: microfone + comando teste som (sem som automatico)");
    ESP_LOGI(TAG, "GPIO BCLK=%d WS=%d MIC=%d AMP=%d",
             IVY_HW04_I2S_BCLK, IVY_HW04_I2S_LRCK,
             IVY_HW04_I2S_DIN, IVY_HW04_I2S_DOUT);
    ESP_ERROR_CHECK(start_mic());
    xTaskCreate(serial_task, "ivy_console", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "Microfone ativo; para o alto-falante envie 'teste som' + ENTER no Serial.");
    while (true) {
        uint32_t peak = 0;
        uint64_t magnitude = 0;
        uint32_t frames = 0;
        for (int block = 0; block < 64; ++block) {
            if (!rx_chan) {
                vTaskDelay(pdMS_TO_TICKS(30));
                continue;
            }
            size_t got = 0;
            esp_err_t err = i2s_channel_read(rx_chan, capture, sizeof(capture),
                                             &got, pdMS_TO_TICKS(500));
            if (err != ESP_OK) continue;
            uint32_t count = (uint32_t)(got / (2 * sizeof(int32_t)));
            for (uint32_t i = 0; i < count; ++i) {
                int32_t sample = capture[i * 2] >> 16;
                uint32_t absval = (uint32_t)(sample < 0 ? -sample : sample);
                if (absval > peak) peak = absval;
                magnitude += absval;
                ++frames;
            }
        }
        if (frames) ESP_LOGI(TAG, "MIC frames=%" PRIu32 " peak=%" PRIu32
             " avg_abs=%" PRIu32, frames, peak, (uint32_t)(magnitude / frames));
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
