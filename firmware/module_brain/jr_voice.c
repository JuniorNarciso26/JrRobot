#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "model_path.h"

#include "jr_audio.h"
#include "jr_board.h"
#include "jr_tts.h"
#include "jr_voice.h"
#if JRBOT_WITH_BABYTALK
#include "jr_stt.h"
#include "jr_usb_terminal.h"
#endif

#define JR_VOICE_BUS_TIMEOUT_MS 2500U
#define JR_VOICE_READ_TIMEOUT_MS 350U
#if JRBOT_WITH_BABYTALK
#define JR_VOICE_TASK_STACK 16384
#else
#define JR_VOICE_TASK_STACK 10240
#endif
#define JR_REPLY_SETTLE_MS 350

static const char *TAG = "jrbot_voice";
static portMUX_TYPE voice_lock = portMUX_INITIALIZER_UNLOCKED;
static TaskHandle_t voice_task_handle = NULL;
static volatile bool stop_requested = false;
static bool bus_owned = false;
static i2s_chan_handle_t rx_chan = NULL;
static srmodel_list_t *models = NULL;

static const esp_wn_iface_t *wakenet = NULL;
static model_iface_data_t *wn_data = NULL;
static char wn_model_name[64] = "not_loaded";

static int32_t *raw_buffer = NULL;
static int16_t *pcm_buffer = NULL;
static jr_voice_wake_cb_t wake_callback = NULL;

typedef enum {
    JR_VOICE_MODE_NONE = 0,
    JR_VOICE_MODE_WAKENET_PT,
} jr_voice_mode_t;

static jr_voice_mode_t voice_mode = JR_VOICE_MODE_NONE;

static jr_voice_status_t voice_status = {
    .running = false,
    .mic_ready = false,
    .frames = 0,
    .detections = 0,
    .last_level = 0,
    .sample_rate = 0,
    .chunk_samples = 0,
    .last_error = ESP_ERR_INVALID_STATE,
    .engine = "not_started",
    .model = "not_loaded",
    .wakeword = "not_loaded",
};

static int abs16(int16_t value) {
    int v = value;
    return v < 0 ? -v : v;
}

static void status_error(esp_err_t err) {
    portENTER_CRITICAL(&voice_lock);
    voice_status.last_error = err;
    portEXIT_CRITICAL(&voice_lock);
}

static void status_mic(bool ready) {
    portENTER_CRITICAL(&voice_lock);
    voice_status.mic_ready = ready;
    portEXIT_CRITICAL(&voice_lock);
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

static void close_mic(void) {
    if (rx_chan) {
        (void)i2s_channel_disable(rx_chan);
        (void)i2s_del_channel(rx_chan);
        rx_chan = NULL;
    }
    release_mic_pins();
    if (bus_owned) {
        jr_audio_bus_release();
        bus_owned = false;
    }
    status_mic(false);
}

static esp_err_t open_mic(int sample_rate) {
    if (!jr_audio_bus_acquire(JR_VOICE_BUS_TIMEOUT_MS)) return ESP_ERR_TIMEOUT;
    bus_owned = true;
    jr_audio_stop();
    quiet_amplifier();
    (void)gpio_set_pull_mode((gpio_num_t)JR_MIC_SD_GPIO, GPIO_PULLDOWN_ONLY);

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    esp_err_t err = i2s_new_channel(&chan_cfg, NULL, &rx_chan);
    if (err != ESP_OK) goto fail;

    i2s_std_config_t cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
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
    err = i2s_channel_init_std_mode(rx_chan, &cfg);
    if (err != ESP_OK) goto fail;
    err = i2s_channel_enable(rx_chan);
    if (err != ESP_OK) goto fail;

    status_mic(true);
    return ESP_OK;

fail:
    if (rx_chan) {
        (void)i2s_del_channel(rx_chan);
        rx_chan = NULL;
    }
    release_mic_pins();
    if (bus_owned) {
        jr_audio_bus_release();
        bus_owned = false;
    }
    status_mic(false);
    return err;
}

static void voice_cleanup(void) {
    close_mic();
#if JRBOT_WITH_BABYTALK
    jr_stt_close();
#endif

    if (wn_data && wakenet) {
        wakenet->destroy(wn_data);
        wn_data = NULL;
    }
    wakenet = NULL;

    if (models) {
        esp_srmodel_deinit(models);
        models = NULL;
    }

    free(raw_buffer);
    raw_buffer = NULL;
    free(pcm_buffer);
    pcm_buffer = NULL;

    voice_mode = JR_VOICE_MODE_NONE;
    portENTER_CRITICAL(&voice_lock);
    voice_status.running = false;
    voice_status.mic_ready = false;
    portEXIT_CRITICAL(&voice_lock);
}

static esp_err_t init_wakenet_ola_esp(int *chunk, int *sample_rate) {
    char *model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);
    if (!model_name) return ESP_ERR_NOT_FOUND;
    // Refuse to silently select an older English wake word from stale sdkconfig.
    if (strcmp(model_name, "wn10_pt_olaesp") != 0) {
        ESP_LOGE(TAG, "WakeNet mismatch: expected=wn10_pt_olaesp found=%s", model_name);
        return ESP_ERR_NOT_SUPPORTED;
    }

    wakenet = esp_wn_handle_from_name(model_name);
    if (!wakenet) return ESP_ERR_NOT_SUPPORTED;

    wn_data = wakenet->create(model_name, DET_MODE_90);
    if (!wn_data) return ESP_ERR_NO_MEM;
    snprintf(wn_model_name, sizeof(wn_model_name), "%s", model_name);

    int wn_chunk = wakenet->get_samp_chunksize(wn_data);
    int wn_rate = wakenet->get_samp_rate(wn_data);
    if (wn_chunk <= 0 || wn_rate <= 0) return ESP_FAIL;

    *chunk = wn_chunk;
    *sample_rate = wn_rate;
    voice_mode = JR_VOICE_MODE_WAKENET_PT;
    ESP_LOGI(TAG, "WakeNet ready model=%s wakeword=Ola_ESP rate=%d chunk=%d",
             wn_model_name, wn_rate, wn_chunk);
    return ESP_OK;
}

static void clean_detector(void) {
    if (voice_mode == JR_VOICE_MODE_WAKENET_PT && wakenet && wn_data) {
        wakenet->clean(wn_data);
    }
}

static esp_err_t reply_and_resume(void) {
    int sample_rate;
    portENTER_CRITICAL(&voice_lock);
    sample_rate = voice_status.sample_rate;
    portEXIT_CRITICAL(&voice_lock);

    /*
     * Preserve the hardware-validated feedback: beep first, then OLED Love
     * for 3 seconds. WakeNet's microphone and speaker share I2S pins,
     * so close the microphone before both audio outputs.
     */
    close_mic();
    ESP_LOGI(TAG, "WAKE_REPLY_BEGIN type=beep frequency=880Hz duration_ms=180");
    esp_err_t beep_err = jr_audio_test_tone(880, 180);
    ESP_LOGI(TAG, "WAKE_REPLY_END type=beep result=%s", esp_err_to_name(beep_err));

#if JRBOT_WITH_BABYTALK
    /* This experiment logs text only: no wake greeting, face command or action. */
    if (beep_err == ESP_OK && !stop_requested) {
        vTaskDelay(pdMS_TO_TICKS(100));
        esp_err_t capture_err = stop_requested ? ESP_ERR_INVALID_STATE : open_mic(sample_rate);
        if (capture_err == ESP_OK) capture_err = jr_stt_capture(rx_chan, &stop_requested);
        close_mic();
        if (capture_err == ESP_OK) jr_stt_transcribe(&stop_requested);
        else ESP_LOGW(TAG, "JR_STT_ERROR capture=%s", esp_err_to_name(capture_err));
    }
#else
    if (beep_err == ESP_OK && !stop_requested && wake_callback) {
        /* Existing OLED Love reaction runs after beep, independently of vTTS. */
        wake_callback("Ola ESP", wn_model_name, 1);
    }

    /* Extra acknowledgement only. A failed synthesis must not remove
     * the beep, face reaction or WakeNet listening recovery. */
    if (!stop_requested && beep_err == ESP_OK) {
        ESP_LOGI(TAG, "WAKE_REPLY_BEGIN type=vtts phrase=ola_sou_o_jrbot");
        esp_err_t tts_err = jr_tts_reply_to_wake("Olá! Eu sou o JrBot.");
        ESP_LOGI(TAG, "WAKE_REPLY_END type=vtts result=%s", esp_err_to_name(tts_err));
        if (tts_err != ESP_OK) {
            ESP_LOGW(TAG, "WAKE_REPLY_VTTS_FAILED beep_and_face_preserved=1 error=%s",
                     esp_err_to_name(tts_err));
        }
    }

#endif
    vTaskDelay(pdMS_TO_TICKS(JR_REPLY_SETTLE_MS));
    if (stop_requested) return beep_err;

    esp_err_t reopen = open_mic(sample_rate);
    if (reopen != ESP_OK) {
        ESP_LOGE(TAG, "reply completed but mic resume failed: %s", esp_err_to_name(reopen));
        return reopen;
    }

    clean_detector();
#if JRBOT_WITH_BABYTALK
    jr_usb_terminal_emit("JR_STT_WAIT_WAKE: diga Ola ESP para nova captura");
#endif
    ESP_LOGI(TAG, "WAKE_LISTENING_RESUMED result=%s listening_resumed=1",
             esp_err_to_name(beep_err));
    return beep_err;
}

static bool handle_wakenet_frame(void) {
    wakenet_state_t detected = wakenet->detect(wn_data, pcm_buffer);
    if (detected != WAKENET_DETECTED) return false;

    uint32_t count;
    portENTER_CRITICAL(&voice_lock);
    voice_status.detections++;
    count = voice_status.detections;
    portEXIT_CRITICAL(&voice_lock);

    ESP_LOGI(TAG, "WAKEWORD_DETECTED keyword=Ola_ESP model=%s count=%lu",
             wn_model_name, (unsigned long)count);
    return true;
}

static void voice_task(void *arg) {
    (void)arg;
    const int chunk = voice_status.chunk_samples;

    while (!stop_requested) {
        size_t bytes_read = 0;
        esp_err_t err = i2s_channel_read(rx_chan, raw_buffer,
                                         (size_t)chunk * sizeof(int32_t),
                                         &bytes_read,
                                         pdMS_TO_TICKS(JR_VOICE_READ_TIMEOUT_MS));
        if (err == ESP_ERR_TIMEOUT) continue;
        if (err != ESP_OK) {
            status_error(err);
            ESP_LOGE(TAG, "I2S read failed: %s", esp_err_to_name(err));
            break;
        }

        int frames = (int)(bytes_read / sizeof(int32_t));
        if (frames != chunk) continue;

        uint64_t level_sum = 0;
        for (int i = 0; i < chunk; ++i) {
            // Preserve the gain proven by HW04 WakeNet tests; record clipping for tuning.
            int32_t scaled = raw_buffer[i] >> 14;
            if (scaled > 32767) scaled = 32767;
            if (scaled < -32768) scaled = -32768;
            pcm_buffer[i] = (int16_t)scaled;
            level_sum += (uint32_t)abs16(pcm_buffer[i]);
        }

        portENTER_CRITICAL(&voice_lock);
        voice_status.frames += (uint32_t)chunk;
        voice_status.last_level = chunk ? (uint32_t)(level_sum / (uint64_t)chunk) : 0;
        portEXIT_CRITICAL(&voice_lock);

        bool detected = (voice_mode == JR_VOICE_MODE_WAKENET_PT) && handle_wakenet_frame();

        if (detected) {
            esp_err_t reply_err = reply_and_resume();
            if (stop_requested) break;
            if (reply_err != ESP_OK) {
                status_error(reply_err);
                if (!rx_chan) break;
            } else {
                status_error(ESP_OK);
            }
        }
    }

    voice_cleanup();
    portENTER_CRITICAL(&voice_lock);
    voice_task_handle = NULL;
    portEXIT_CRITICAL(&voice_lock);
    vTaskDelete(NULL);
}

esp_err_t jr_voice_start(jr_voice_wake_cb_t wake_cb) {
    portENTER_CRITICAL(&voice_lock);
    bool already_running = voice_status.running;
    portEXIT_CRITICAL(&voice_lock);
    if (already_running) return stop_requested ? ESP_ERR_INVALID_STATE : ESP_OK;

    stop_requested = false;
    wake_callback = wake_cb;

#if JRBOT_WITH_BABYTALK
    esp_err_t stt_err = jr_stt_open();
    if (stt_err != ESP_OK) { status_error(stt_err); return stt_err; }
#endif
    models = esp_srmodel_init("model");
    if (!models) {
        status_error(ESP_ERR_NOT_FOUND);
        voice_cleanup();
        return ESP_ERR_NOT_FOUND;
    }

    int chunk = 0;
    int sample_rate = 0;
    // Initialize only the validated Portuguese wake word.
    esp_err_t err = init_wakenet_ola_esp(&chunk, &sample_rate);
    if (err != ESP_OK) {
        status_error(err);
        voice_cleanup();
        return err;
    }
    if (sample_rate != 16000) {
        status_error(ESP_ERR_NOT_SUPPORTED);
        voice_cleanup();
        return ESP_ERR_NOT_SUPPORTED;
    }

    raw_buffer = calloc((size_t)chunk, sizeof(int32_t));
    pcm_buffer = calloc((size_t)chunk, sizeof(int16_t));
    if (!raw_buffer || !pcm_buffer) {
        status_error(ESP_ERR_NO_MEM);
        voice_cleanup();
        return ESP_ERR_NO_MEM;
    }

    err = open_mic(sample_rate);
    if (err != ESP_OK) {
        status_error(err);
        voice_cleanup();
        return err;
    }

    portENTER_CRITICAL(&voice_lock);
    voice_status.running = true;
    voice_status.mic_ready = true;
    voice_status.frames = 0;
    voice_status.detections = 0;
    voice_status.last_level = 0;
    voice_status.sample_rate = sample_rate;
    voice_status.chunk_samples = chunk;
    voice_status.last_error = ESP_OK;
    snprintf(voice_status.engine, sizeof(voice_status.engine), "%s", "esp-sr-wakenet");
    snprintf(voice_status.model, sizeof(voice_status.model), "%s", wn_model_name);
    snprintf(voice_status.wakeword, sizeof(voice_status.wakeword), "%s", "Ola ESP");
    portEXIT_CRITICAL(&voice_lock);

    if (xTaskCreatePinnedToCore(voice_task, "jr_voice", JR_VOICE_TASK_STACK, NULL, 5,
                                &voice_task_handle, 1) != pdPASS) {
        status_error(ESP_ERR_NO_MEM);
        voice_cleanup();
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "voice engine ready engine=%s model=%s keyword=%s rate=%d chunk=%d",
             voice_status.engine, voice_status.model, voice_status.wakeword, sample_rate, chunk);
    /* Prepare the short fixed acknowledgement while WakeNet listens. No I2S
     * acquisition or blocking synthesis is performed in this startup path. */
#if !JRBOT_WITH_BABYTALK
    jr_tts_prepare_wake_reply();
#endif
    return ESP_OK;
}

void jr_voice_stop(void) {
    portENTER_CRITICAL(&voice_lock);
    TaskHandle_t task = voice_task_handle;
    portEXIT_CRITICAL(&voice_lock);
    if (!task) {
        voice_cleanup();
        return;
    }

    stop_requested = true;
    for (int i = 0; i < 40; ++i) {
        portENTER_CRITICAL(&voice_lock);
        bool done = voice_task_handle == NULL;
        portEXIT_CRITICAL(&voice_lock);
        if (done) break;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void jr_voice_get_status(jr_voice_status_t *out) {
    if (!out) return;
    portENTER_CRITICAL(&voice_lock);
    *out = voice_status;
    portEXIT_CRITICAL(&voice_lock);
}
