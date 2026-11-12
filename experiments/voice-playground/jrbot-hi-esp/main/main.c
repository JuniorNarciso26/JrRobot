/*
 * JrBot HW04 audio capture adapter for Espressif WakeNet / ESP-SR.
 * The official ESP-Skainet examples remain unmodified in ../esp-skainet.
 * Only the I2S input is adapted here for the MS3625 microphone.
 */
#include <stdint.h>
#include <stdlib.h>

#include "driver/i2s_std.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "model_path.h"

#define JR_MIC_SCK GPIO_NUM_21
#define JR_MIC_WS  GPIO_NUM_47
#define JR_MIC_SD  GPIO_NUM_41

static const char *TAG = "jr_wake";

static esp_err_t jr_mic_init(i2s_chan_handle_t *rx, int sample_rate)
{
    i2s_chan_config_t channel = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    esp_err_t err = i2s_new_channel(&channel, NULL, rx);
    if (err != ESP_OK) return err;

    i2s_std_config_t config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = JR_MIC_SCK,
            .ws = JR_MIC_WS,
            .dout = I2S_GPIO_UNUSED,
            .din = JR_MIC_SD,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false
            },
        },
    };
    config.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    err = i2s_channel_init_std_mode(*rx, &config);
    if (err == ESP_OK) err = i2s_channel_enable(*rx);
    if (err != ESP_OK) {
        i2s_del_channel(*rx);
        *rx = NULL;
    }
    return err;
}

void app_main(void)
{
    srmodel_list_t *models = esp_srmodel_init("model");
    if (!models) {
        ESP_LOGE(TAG, "MODEL_ERROR: particao model nao disponivel");
        return;
    }
    char *model = esp_srmodel_filter(models, ESP_WN_PREFIX, "hiesp");
    if (!model) {
        ESP_LOGE(TAG, "MODEL_ERROR: Hi ESP nao selecionado em sdkconfig");
        esp_srmodel_deinit(models);
        return;
    }

    const esp_wn_iface_t *wake = esp_wn_handle_from_name(model);
    if (!wake) {
        ESP_LOGE(TAG, "MODEL_ERROR: WakeNet nao carregou");
        esp_srmodel_deinit(models);
        return;
    }
    model_iface_data_t *engine = wake->create(model, DET_MODE_95);
    if (!engine) {
        ESP_LOGE(TAG, "MODEL_ERROR: falha ao inicializar WakeNet");
        esp_srmodel_deinit(models);
        return;
    }

    const int rate = wake->get_samp_rate(engine);
    const int chunk = wake->get_samp_chunksize(engine);
    if (rate <= 0 || chunk <= 0) {
        ESP_LOGE(TAG, "MODEL_ERROR: parametros invalidos");
        wake->destroy(engine);
        esp_srmodel_deinit(models);
        return;
    }

    int32_t *raw = calloc((size_t)chunk, sizeof(*raw));
    int16_t *pcm = calloc((size_t)chunk, sizeof(*pcm));
    i2s_chan_handle_t rx = NULL;
    if (!raw || !pcm) {
        ESP_LOGE(TAG, "MEM_ERROR: buffers I2S");
        goto done;
    }
    if (jr_mic_init(&rx, rate) != ESP_OK) {
        ESP_LOGE(TAG, "MIC_ERROR: falha ao configurar SCK=21 WS=47 SD=41");
        goto done;
    }

    ESP_LOGI(TAG, "MIC_READY SCK=21 WS=47 SD=41 rate=%d model=%s", rate, model);
    ESP_LOGI(TAG, "LISTENING: fale Hi ESP; MIC_LEVEL indica audio captado");
    int64_t last_level_us = esp_timer_get_time();
    uint64_t level_acc = 0;
    unsigned level_samples = 0;
    uint32_t detections = 0;

    while (true) {
        size_t got = 0;
        while (got < (size_t)chunk) {
            size_t bytes = 0;
            esp_err_t err = i2s_channel_read(
                rx, raw + got, ((size_t)chunk - got) * sizeof(int32_t),
                &bytes, pdMS_TO_TICKS(500));
            if (err != ESP_OK || bytes == 0 || bytes % sizeof(int32_t) != 0) {
                ESP_LOGE(TAG, "MIC_ERROR: leitura I2S falhou: %s", esp_err_to_name(err));
                goto done;
            }
            got += bytes / sizeof(int32_t);
        }
        for (int i = 0; i < chunk; ++i) {
            int32_t sample = raw[i] >> 14; // mesmo ajuste do firmware JrBot atual
            if (sample > INT16_MAX) sample = INT16_MAX;
            if (sample < INT16_MIN) sample = INT16_MIN;
            pcm[i] = (int16_t)sample;
            level_acc += (sample < 0) ? (uint32_t)(-sample) : (uint32_t)sample;
        }
        level_samples += (unsigned)chunk;

        if (wake->detect(engine, pcm) == WAKENET_DETECTED) {
            ++detections;
            ESP_LOGW(TAG, "HI_ESP_DETECTED count=%lu", (unsigned long)detections);
        }
        int64_t now = esp_timer_get_time();
        if (now - last_level_us >= 2000000) {
            ESP_LOGI(TAG, "MIC_LEVEL average=%lu",
                     (unsigned long)(level_samples ? level_acc / level_samples : 0));
            level_acc = 0;
            level_samples = 0;
            last_level_us = now;
        }
    }

done:
    if (rx) {
        i2s_channel_disable(rx);
        i2s_del_channel(rx);
    }
    free(raw);
    free(pcm);
    wake->destroy(engine);
    esp_srmodel_deinit(models);
}
