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
#define SAMPLES (RATE * 10)
#define PREROLL (RATE / 5)
static int mic_gain = 1;
static int speech_threshold = 300;
static int pause_ms = 800;
static const char *TAG = "JR_BABYTALK";

static void memory(void) {
    ESP_LOGI(TAG, "heap_internal=%u largest_internal=%u psram_free=%u",
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

static bool enter_pressed(void) {
    bool enter = false;
    for (int i=0; i<64; ++i) {
        int ch=getchar();
        if (ch==EOF) { clearerr(stdin); break; }
        if (ch=='\n') enter=true;
    }
    return enter;
}

static esp_err_t capture(int16_t *pcm, int *samples) {
    *samples=0;
    (void)enter_pressed(); // discard pending CR/LF from the start command
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
    ESP_LOGI(TAG, "FALE AGORA: ENTER encerra | pausa=%dms | limiar_RMS=%d | ganho=%dx | max=10s",
        pause_ms, speech_threshold, mic_gain);
    bool voice=false;
    unsigned attack=0, silence=0, waited=0;
    const char *reason="limite";
    const int64_t started=esp_timer_get_time();
    while (count < SAMPLES) {
        if (enter_pressed() && esp_timer_get_time()-started>200000) { reason="enter"; break; }
        size_t bytes = 0;
        err = i2s_channel_read(rx, raw, sizeof(raw), &bytes, 500);
        if (err != ESP_OK || !bytes || bytes % sizeof(int32_t) || esp_timer_get_time()-started>17000000) {
            err = ESP_FAIL;
            goto done;
        }
        const size_t frames=bytes/sizeof(int32_t);
        int16_t block[256];
        if (frames>256) { err=ESP_FAIL; goto done; }
        uint64_t energy=0;
        for (size_t j=0; j<frames; ++j) {
            int32_t value = (raw[j] >> 16) * mic_gain;
            if (value > 32767) { value=32767; ++clipped; }
            if (value < -32768) { value=-32768; ++clipped; }
            block[j] = (int16_t)value;
            energy += (int64_t)value*value;
            int magnitude = value < 0 ? -value : value;
            if (magnitude > peak) peak = magnitude;
        }
        bool loud=energy > (uint64_t)speech_threshold*speech_threshold*frames;
        attack=loud ? attack+1 : 0;
        if (!voice && attack>=3) { voice=true; ESP_LOGI(TAG,"FALA_DETECTADA"); }
        if (!voice && count+frames>PREROLL) {
            const size_t drop=count+frames-PREROLL;
            memmove(pcm,pcm+drop,(count-drop)*sizeof(int16_t));
            count-=drop;
        }
        size_t take=frames < SAMPLES-count ? frames : SAMPLES-count;
        memcpy(pcm+count,block,take*sizeof(int16_t));
        count+=take;
        if (!voice) {
            waited+=frames;
            if (waited>=RATE*5) { reason="sem_fala"; break; }
        } else {
            silence=loud ? 0 : silence+frames;
            if (silence >= (unsigned)(RATE*pause_ms/1000)) { reason="pausa"; break; }
        }
    }
    *samples=voice ? (int)count : 0;
    ESP_LOGI(TAG, "CAPTURADO samples=%u peak=%d clipped=%d motivo=%s fala=%d",
        (unsigned)*samples,peak,clipped,reason,(int)voice);

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
    int64_t ignore_enter_until=0;
    puts("JR_STT_READY: ENTER inicia; outro ENTER ou uma pausa encerra. ajuda mostra controles.");
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
        if (!discard && (!strcmp(line,"ouvir") || !strcmp(line,"listen") || (!line[0] && esp_timer_get_time()>=ignore_enter_until))) {
            int n_samples=0;
            if (capture(pcm, &n_samples) != ESP_OK) ESP_LOGE(TAG,"Captura incompleta; nao transcrita");
            else if (n_samples < RATE/5) { puts("JR_STT_NO_SPEECH: sem fala suficiente; ajuste sensibilidade se necessario."); }
            else {
                puts("JR_STT_PROCESSING: processando no ESP32..."); fflush(stdout);
                stt_result_t result;
                const int64_t start = esp_timer_get_time();
                rc = stt_engine_run(pcm, n_samples, &result);
                const float ms = (esp_timer_get_time()-start)/1000.0f;
                if (rc) ESP_LOGE(TAG,"Falha transcricao rc=%d",rc);
                else {
                    printf("JR_STT_TEXT: %s\n",result.text[0] ? result.text : "(vazio)");
                    printf("JR_STT_TIME: total_ms=%.1f fe_ms=%.1f model_ms=%.1f dec_ms=%.1f frames=%d\n",
                        (double)ms,(double)result.fe_ms,(double)result.model_ms,(double)result.dec_ms,result.frames);
                }
                memory();
            }
            ignore_enter_until=esp_timer_get_time()+300000;
            puts("JR_STT_READY: ENTER inicia nova escuta; ajuda mostra controles."); fflush(stdout);
        } else if (!discard && line[0]) {
            int value=0;
            char extra=0;
            if (sscanf(line,"ganho %d %c",&value,&extra)==1 && (value==1 || value==2 || value==4 || value==8)) mic_gain=value;
            else if (sscanf(line,"sensibilidade %d %c",&value,&extra)==1 && value>=50 && value<=5000) speech_threshold=value;
            else if (sscanf(line,"pausa %d %c",&value,&extra)==1 && value>=400 && value<=3000) pause_ms=value;
            else puts("ENTER: iniciar/parar | ganho 1/2/4/8 | sensibilidade 50..5000 (MENOR=mais sensivel) | pausa 400..3000 (ms)");
            printf("JR_STT_CONFIG ganho=%d limiar_RMS=%d pausa_ms=%d\n",mic_gain,speech_threshold,pause_ms);
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
