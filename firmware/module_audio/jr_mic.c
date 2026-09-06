#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_err.h"

#include "jr_board.h"
#include "jr_mic.h"

#define JR_MIC_SAMPLE_RATE 16000
#define JR_MIC_FRAMES 256

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

bool jr_mic_probe(jr_mic_status_t *out) {
    jr_mic_status_t result = {.present=false,.samples=0,.peak_raw=0,.changes=0,.channel='L',.last_error=ESP_ERR_INVALID_STATE};
    if (!JR_MIC_ENABLED) {
        result.last_error = ESP_ERR_NOT_SUPPORTED;
        if (out) *out = result;
        return false;
    }

    /* Keep the amplifier silent while BCLK/WS are temporarily used by the mic RX probe. */
    (void)gpio_set_direction((gpio_num_t)JR_AUDIO_DIN_GPIO, GPIO_MODE_OUTPUT);
    (void)gpio_set_level((gpio_num_t)JR_AUDIO_DIN_GPIO, 0);
    (void)gpio_set_pull_mode((gpio_num_t)JR_MIC_SD_GPIO, GPIO_PULLDOWN_ONLY);

    i2s_chan_handle_t rx = NULL;
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    esp_err_t err = i2s_new_channel(&chan_cfg, NULL, &rx);
    if (err != ESP_OK) goto finish;

    i2s_std_config_t cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(JR_MIC_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = JR_MIC_SCK_GPIO,
            .ws = JR_MIC_WS_GPIO,
            .dout = I2S_GPIO_UNUSED,
            .din = JR_MIC_SD_GPIO,
            .invert_flags = {.mclk_inv=false,.bclk_inv=false,.ws_inv=false},
        },
    };
    err = i2s_channel_init_std_mode(rx, &cfg);
    if (err != ESP_OK) goto finish;
    err = i2s_channel_enable(rx);
    if (err != ESP_OK) goto finish;

    int32_t samples[JR_MIC_FRAMES * 2];
    memset(samples, 0, sizeof(samples));
    size_t bytes_read = 0;
    err = i2s_channel_read(rx, samples, sizeof(samples), &bytes_read, 500);
    if (err != ESP_OK) goto finish;

    unsigned frames = (unsigned)(bytes_read / (sizeof(int32_t) * 2));
    uint32_t peak[2] = {0,0};
    unsigned nonzero[2] = {0,0}, changes[2] = {0,0};
    int32_t prev[2] = {0,0};
    for (unsigned f=0; f<frames; ++f) {
        for (unsigned ch=0; ch<2; ++ch) {
            int32_t v = samples[f*2+ch];
            uint32_t mag = magnitude32(v);
            if (mag > peak[ch]) peak[ch] = mag;
            if (mag > 16) nonzero[ch]++;
            if (f && difference32(v,prev[ch]) > 16) changes[ch]++;
            prev[ch] = v;
        }
    }
    unsigned chosen = (peak[1] + changes[1] > peak[0] + changes[0]) ? 1U : 0U;
    result.samples = frames;
    result.peak_raw = peak[chosen];
    result.changes = changes[chosen];
    result.channel = chosen ? 'R' : 'L';
    result.present = frames >= 32 && peak[chosen] > 256 && nonzero[chosen] >= 8 && changes[chosen] >= 4;
    result.last_error = ESP_OK;

finish:
    if (rx) {
        (void)i2s_channel_disable(rx);
        (void)i2s_del_channel(rx);
    }
    if (err != ESP_OK) result.last_error = err;
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
