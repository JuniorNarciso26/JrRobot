#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_mn_speech_commands.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "model_path.h"

#include "jr_audio.h"
#include "jr_board.h"
#include "jr_reply_audio.h"
#include "jr_voice.h"

#define JR_VOICE_BUS_TIMEOUT_MS 2500U
#define JR_VOICE_READ_TIMEOUT_MS 350U
#define JR_VOICE_TASK_STACK 10240
#define JR_MN_DURATION_MS 5000
#define JR_NAME_COMMAND_ID 1
#define JR_NAME_MIN_PROBABILITY 0.70f
#define JR_REPLY_SETTLE_MS 220

static const char *TAG = "jrbot_voice";
static portMUX_TYPE voice_lock = portMUX_INITIALIZER_UNLOCKED;
static TaskHandle_t voice_task_handle = NULL;
static volatile bool stop_requested = false;
static bool bus_owned = false;
static i2s_chan_handle_t rx_chan = NULL;
static srmodel_list_t *models = NULL;

static esp_mn_iface_t *multinet = NULL;
static model_iface_data_t *mn_data = NULL;
static bool mn_commands_ready = false;
static char mn_model_name[64] = "not_loaded";

static int32_t *raw_buffer = NULL;
static int16_t *pcm_buffer = NULL;
static jr_voice_wake_cb_t wake_callback = NULL;

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

    if (mn_commands_ready) {
        (void)esp_mn_commands_free();
        mn_commands_ready = false;
    }
    if (mn_data && multinet) {
        multinet->destroy(mn_data);
        mn_data = NULL;
    }
    multinet = NULL;

    if (models) {
        esp_srmodel_deinit(models);
        models = NULL;
    }

    free(raw_buffer);
    raw_buffer = NULL;
    free(pcm_buffer);
    pcm_buffer = NULL;

    portENTER_CRITICAL(&voice_lock);
    voice_status.running = false;
    voice_status.mic_ready = false;
    portEXIT_CRITICAL(&voice_lock);
}

static esp_err_t init_multinet_name(int *chunk, int *sample_rate) {
    char *model_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_ENGLISH);
    if (!model_name) return ESP_ERR_NOT_FOUND;

    multinet = esp_mn_handle_from_name(model_name);
    if (!multinet) return ESP_ERR_NOT_SUPPORTED;

    mn_data = multinet->create(model_name, JR_MN_DURATION_MS);
    if (!mn_data) return ESP_ERR_NO_MEM;
    snprintf(mn_model_name, sizeof(mn_model_name), "%s", model_name);

    esp_err_t err = esp_mn_commands_alloc(multinet, mn_data);
    if (err != ESP_OK) return err;
    mn_commands_ready = true;
    (void)esp_mn_commands_clear();

    err = esp_mn_commands_add(JR_NAME_COMMAND_ID, "JR BOT");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MultiNet rejected JR BOT error=%s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "MultiNet command accepted: JR BOT");

    esp_mn_error_t *command_errors = esp_mn_commands_update();
    if (command_errors && command_errors->num > 0) {
        ESP_LOGE(TAG, "MultiNet could not activate JR BOT update_errors=%d", command_errors->num);
        return ESP_ERR_INVALID_ARG;
    }

    if (multinet->set_det_threshold) {
        int threshold_result = multinet->set_det_threshold(mn_data, JR_NAME_MIN_PROBABILITY);
        ESP_LOGI(TAG, "JR BOT detection threshold=%.2f result=%d",
                 (double)JR_NAME_MIN_PROBABILITY, threshold_result);
    }

    int mn_chunk = multinet->get_samp_chunksize(mn_data);
    int mn_rate = multinet->get_samp_rate(mn_data);
    if (mn_chunk <= 0 || mn_rate <= 0) return ESP_FAIL;

    *chunk = mn_chunk;
    *sample_rate = mn_rate;
    ESP_LOGI(TAG,
             "direct JrBot recognizer ready model=%s command=JR_BOT min_probability=%.2f rate=%d chunk=%d",
             mn_model_name, (double)JR_NAME_MIN_PROBABILITY, mn_rate, mn_chunk);
    return ESP_OK;
}

static void clean_detector(void) {
    if (multinet && mn_data) multinet->clean(mn_data);
}

static esp_err_t reply_and_resume(void) {
    int sample_rate;
    portENTER_CRITICAL(&voice_lock);
    sample_rate = voice_status.sample_rate;
    portEXIT_CRITICAL(&voice_lock);

    close_mic();
    ESP_LOGI(TAG, "reply begin type=spoken_oi");

    esp_err_t reply_err = jr_reply_play_oi();
    const char *reply_type = "spoken_oi";
    if (reply_err != ESP_OK) {
        ESP_LOGW(TAG, "spoken reply failed: %s; using chirp fallback", esp_err_to_name(reply_err));
        reply_type = "chirp_fallback";
        reply_err = jr_audio_test_tone(1040, 180);
    }

    vTaskDelay(pdMS_TO_TICKS(JR_REPLY_SETTLE_MS));
    if (stop_requested) return reply_err;

    esp_err_t reopen = open_mic(sample_rate);
    if (reopen != ESP_OK) {
        ESP_LOGE(TAG, "reply completed but mic resume failed: %s", esp_err_to_name(reopen));
        return reopen;
    }

    clean_detector();
    ESP_LOGI(TAG, "reply end type=%s result=%s listening_resumed=1",
             reply_type, esp_err_to_name(reply_err));
    return reply_err;
}

static bool handle_multinet_frame(void) {
    esp_mn_state_t state = multinet->detect(mn_data, pcm_buffer);
    if (state == ESP_MN_STATE_TIMEOUT) {
        multinet->clean(mn_data);
        return false;
    }
    if (state != ESP_MN_STATE_DETECTED) return false;

    esp_mn_results_t *result = multinet->get_results(mn_data);
    if (!result || result->num <= 0 || result->command_id[0] != JR_NAME_COMMAND_ID) {
        multinet->clean(mn_data);
        return false;
    }

    float probability = result->prob[0];
    const char *recognized = result->string[0] ? result->string : "JR BOT";
    if (probability < JR_NAME_MIN_PROBABILITY) {
        ESP_LOGI(TAG,
                 "JR BOT candidate ignored recognized=\"%s\" probability=%.3f min=%.2f",
                 recognized, (double)probability, (double)JR_NAME_MIN_PROBABILITY);
        multinet->clean(mn_data);
        return false;
    }

    jr_voice_wake_cb_t callback;
    uint32_t count;
    portENTER_CRITICAL(&voice_lock);
    voice_status.detections++;
    count = voice_status.detections;
    callback = wake_callback;
    portEXIT_CRITICAL(&voice_lock);

    ESP_LOGI(TAG,
             "name detected keyword=JrBot recognized=\"%s\" model=%s probability=%.3f count=%lu",
             recognized, mn_model_name, (double)probability, (unsigned long)count);
    if (callback) callback("JrBot", mn_model_name, JR_NAME_COMMAND_ID);
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

        if (handle_multinet_frame()) {
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
    if (already_running) return ESP_OK;

    stop_requested = false;
    wake_callback = wake_cb;

    models = esp_srmodel_init("model");
    if (!models) {
        status_error(ESP_ERR_NOT_FOUND);
        return ESP_ERR_NOT_FOUND;
    }

    int chunk = 0;
    int sample_rate = 0;
    esp_err_t err = init_multinet_name(&chunk, &sample_rate);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "JrBot recognizer init failed: %s", esp_err_to_name(err));
        status_error(err);
        voice_cleanup();
        return err;
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
    snprintf(voice_status.engine, sizeof(voice_status.engine), "%s", "esp-sr-multinet");
    snprintf(voice_status.model, sizeof(voice_status.model), "%s", mn_model_name);
    snprintf(voice_status.wakeword, sizeof(voice_status.wakeword), "%s", "JrBot");
    portEXIT_CRITICAL(&voice_lock);

    if (xTaskCreatePinnedToCore(voice_task, "jr_voice", JR_VOICE_TASK_STACK, NULL, 5,
                                &voice_task_handle, 1) != pdPASS) {
        status_error(ESP_ERR_NO_MEM);
        voice_cleanup();
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "voice engine ready engine=%s model=%s keyword=%s rate=%d chunk=%d",
             voice_status.engine, voice_status.model, voice_status.wakeword, sample_rate, chunk);
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
