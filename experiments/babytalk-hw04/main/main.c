#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sram_pool.h"
#include "stt_engine.h"

#define RATE 16000
#define SAMPLES (RATE * 3)
static const char *TAG = "JR_BABYTALK";

static void memory(void) {
    ESP_LOGI(TAG, "heap_internal=%u largest_internal=%u psram_free=%u",
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

static esp_err_t capture(int16_t *pcm) {
    i2s_chan_handle_t rx = NULL;
    i2s_chan_config_t channel = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    esp_err_t err = i2s_new_channel(&channel, NULL, &rx);
    if (err != ESP_OK) return err;
    bool enabled = false;
    i2s_std_config_t config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {.mclk=I2S_GPIO_UNUSED, .bclk=GPIO_NUM_21, .ws=GPIO_NUM_47,
                     .dout=I2S_GPIO_UNUSED, .din=GPIO_NUM_41},
    };
    config.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    err = i2s_channel_init_std_mode(rx, &config);
    if (err != ESP_OK) goto done;
    err = i2s_channel_enable(rx);
    if (err != ESP_OK) goto done;
    enabled = true;
    int32_t raw[256];
    size_t count = 0;
    int peak = 0, clipped = 0;
    // Discard initial peripheral/microphone transient before inviting speech.
    for (int n = 0; n < 8; ++n) {
        size_t bytes = 0;
        err = i2s_channel_read(rx, raw, sizeof(raw), &bytes, 500);
        if (err != ESP_OK || bytes != sizeof(raw)) { err = ESP_FAIL; goto done; }
    }
    ESP_LOGI(TAG, "FALE AGORA: 3 segundos, ingles (happy, hello robot, turn on the light)");
    const int64_t deadline = esp_timer_get_time() + 6000000;
    while (count < SAMPLES) {
        size_t bytes = 0;
        err = i2s_channel_read(rx, raw, sizeof(raw), &bytes, 500);
        if (err != ESP_OK || !bytes || bytes % sizeof(int32_t) || esp_timer_get_time() > deadline) {
            err = ESP_FAIL;
            goto done;
        }
        for (size_t j=0; j<bytes/sizeof(int32_t) && count<SAMPLES; ++j) {
            int32_t value = raw[j] >> 14; // same x4 scaling as validated HW04 microphone
            if (value > 32767) { value=32767; ++clipped; }
            if (value < -32768) { value=-32768; ++clipped; }
            pcm[count++] = (int16_t)value;
            int magnitude = value < 0 ? -value : value;
            if (magnitude > peak) peak = magnitude;
        }
    }
    ESP_LOGI(TAG, "CAPTURADO samples=%u peak=%d clipped=%d", (unsigned)count, peak, clipped);
done:
    if (enabled) (void)i2s_channel_disable(rx);
    (void)i2s_del_channel(rx);
    return err;
}

static void listen_task(void *unused) {
    (void)unused;
    int rc = stt_engine_open();
    if (rc) { ESP_LOGE(TAG, "Falha modelo rc=%d; confira modelo/PSRAM", rc); goto stop; }
    stt_info_t info;
    stt_engine_info(&info);
    ESP_LOGI(TAG, "MODELO int4_bytes=%u version=%d ops=%d int4_ops=%d",
        (unsigned)info.model_bytes, info.version, info.ops, info.int4_ops);
    int16_t *pcm = heap_caps_malloc(SAMPLES*sizeof(int16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!pcm) { ESP_LOGE(TAG, "Sem PSRAM para captura"); goto stop; }
    setvbuf(stdin, NULL, _IONBF, 0);
    char line[48] = {0};
    size_t used = 0;
    bool discard = false;
    memory();
    puts("JR_STT_READY: digite ouvir e ENTER para capturar 3s. ajuda mostra instrucoes.");
    fflush(stdout);
    while (true) {
        int ch = getchar();
        if (ch == EOF) { clearerr(stdin); vTaskDelay(pdMS_TO_TICKS(10)); continue; }
        if (ch == '\r') continue;
        if (ch != '\n') {
            if (!discard && used+1<sizeof(line)) line[used++] = (char)ch;
            else discard=true;
            continue;
        }
        line[used]=0;
        if (!discard && (!strcmp(line,"ouvir") || !strcmp(line,"listen"))) {
            if (capture(pcm) != ESP_OK) ESP_LOGE(TAG,"Captura incompleta; nao transcrita");
            else {
                puts("JR_STT_PROCESSING: processando no ESP32..."); fflush(stdout);
                stt_result_t result;
                const int64_t start = esp_timer_get_time();
                rc = stt_engine_run(pcm, SAMPLES, &result);
                const float ms = (esp_timer_get_time()-start)/1000.0f;
                if (rc) ESP_LOGE(TAG,"Falha transcricao rc=%d",rc);
                else {
                    printf("JR_STT_TEXT: %s\n",result.text[0] ? result.text : "(vazio)");
                    printf("JR_STT_TIME: total_ms=%.1f fe_ms=%.1f model_ms=%.1f dec_ms=%.1f frames=%d\n",
                        (double)ms,(double)result.fe_ms,(double)result.model_ms,(double)result.dec_ms,result.frames);
                }
                memory();
            }
            puts("JR_STT_READY: digite ouvir e ENTER para nova tentativa."); fflush(stdout);
        } else if (!discard && (!strcmp(line,"ajuda") || !strcmp(line,"help") || !line[0])) {
            puts("ouvir + ENTER -> aguarde FALE AGORA -> fale em ingles -> leia JR_STT_TEXT.");
            fflush(stdout);
        }
        used=0; discard=false;
    }
stop:
    memory();
    vTaskDelete(NULL);
}

void app_main(void) {
    ESP_LOGI(TAG,"JrBot_V2.2_BABYTALK_TEST | BCLK=21 WS=47 MIC=41 | STT ingles offline");
    gpio_reset_pin(GPIO_NUM_42);
    gpio_set_direction(GPIO_NUM_42, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_42,0); // no speaker output in this experiment
    gpio_set_pull_mode(GPIO_NUM_41, GPIO_PULLDOWN_ONLY);
    if (!heap_caps_get_total_size(MALLOC_CAP_SPIRAM) || sram_pool_reserve()) {
        ESP_LOGE(TAG,"PSRAM ausente ou reserva SRAM falhou"); return;
    }
    if (xTaskCreatePinnedToCore(listen_task,"jr_stt",16*1024,NULL,1,NULL,1)!=pdPASS)
        ESP_LOGE(TAG,"Nao foi possivel criar tarefa STT");
}
