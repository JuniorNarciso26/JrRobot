#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jr_board.h"
#include "jr_camera_diag.h"
#if JR_CAMERA_ENABLED
#include "esp_camera.h"
#include "esp_heap_caps.h"
#include "esp_psram.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

static bool cleanup_failed;
static bool camera_busy;
static portMUX_TYPE camera_state_lock = portMUX_INITIALIZER_UNLOCKED;

static bool camera_acquire(void) {
    bool acquired = false;
    portENTER_CRITICAL(&camera_state_lock);
    if (!camera_busy) {
        camera_busy = true;
        acquired = true;
    }
    portEXIT_CRITICAL(&camera_state_lock);
    return acquired;
}

static void camera_release(void) {
    portENTER_CRITICAL(&camera_state_lock);
    camera_busy = false;
    portEXIT_CRITICAL(&camera_state_lock);
}

static const camera_config_t diagnostic_config = {
    .pin_pwdn = JR_CAM_PWDN, .pin_reset = JR_CAM_RESET,
    .pin_xclk = JR_CAM_XCLK, .pin_sccb_sda = JR_CAM_SDA, .pin_sccb_scl = JR_CAM_SCL,
    .pin_d0 = JR_CAM_D0, .pin_d1 = JR_CAM_D1, .pin_d2 = JR_CAM_D2, .pin_d3 = JR_CAM_D3,
    .pin_d4 = JR_CAM_D4, .pin_d5 = JR_CAM_D5, .pin_d6 = JR_CAM_D6, .pin_d7 = JR_CAM_D7,
    .pin_vsync = JR_CAM_VSYNC, .pin_href = JR_CAM_HREF, .pin_pclk = JR_CAM_PCLK,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0, .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG, .frame_size = FRAMESIZE_QVGA,
    .jpeg_quality = 12, .fb_count = 1,
    .fb_location = CAMERA_FB_IN_PSRAM, .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};
#endif

bool jr_camera_probe_once(unsigned *pid_out) {
    if (pid_out) *pid_out = 0;
#if !JR_CAMERA_ENABLED
    return false;
#else
    if (cleanup_failed || !esp_psram_is_initialized() || !camera_acquire()) return false;
    esp_err_t err = esp_camera_init(&diagnostic_config);
    if (err != ESP_OK) {
        camera_release();
        return false;
    }
    sensor_t *sensor = esp_camera_sensor_get();
    bool present = sensor != NULL;
    if (present && pid_out) *pid_out = sensor->id.PID;
    err = esp_camera_deinit();
    if (err != ESP_OK) {
        cleanup_failed = true;
        present = false;
    }
    camera_release();
    return present;
#endif
}

void jr_camera_jpeg_release(jr_camera_jpeg_t *frame) {
    if (!frame) return;
    if (frame->data) heap_caps_free(frame->data);
    memset(frame, 0, sizeof(*frame));
}

esp_err_t jr_camera_capture_jpeg(jr_camera_jpeg_t *out) {
    if (!out) return ESP_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));
#if !JR_CAMERA_ENABLED
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (cleanup_failed) return ESP_ERR_INVALID_STATE;
    if (!esp_psram_is_initialized()) return ESP_ERR_NOT_SUPPORTED;
    if (!camera_acquire()) return ESP_ERR_INVALID_STATE;

    esp_err_t result = ESP_FAIL;
    bool initialized = false;
    camera_fb_t *fb = NULL;

    esp_err_t err = esp_camera_init(&diagnostic_config);
    if (err != ESP_OK) {
        result = err;
        goto done;
    }
    initialized = true;

    sensor_t *sensor = esp_camera_sensor_get();
    if (!sensor) {
        result = ESP_ERR_NOT_FOUND;
        goto done;
    }

    fb = esp_camera_fb_get();
    if (!fb) {
        result = ESP_ERR_TIMEOUT;
        goto done;
    }
    if (!fb->buf || fb->len < 4 || !fb->width || !fb->height ||
        fb->format != PIXFORMAT_JPEG || fb->buf[0] != 0xff || fb->buf[1] != 0xd8) {
        result = ESP_FAIL;
        goto done;
    }

    out->data = heap_caps_malloc(fb->len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!out->data) out->data = heap_caps_malloc(fb->len, MALLOC_CAP_8BIT);
    if (!out->data) {
        result = ESP_ERR_NO_MEM;
        goto done;
    }
    memcpy(out->data, fb->buf, fb->len);
    out->len = fb->len;
    out->pid = sensor->id.PID;
    out->width = (unsigned)fb->width;
    out->height = (unsigned)fb->height;
    result = ESP_OK;

done:
    if (fb) esp_camera_fb_return(fb);
    if (initialized) {
        err = esp_camera_deinit();
        if (err != ESP_OK) {
            cleanup_failed = true;
            result = err;
        }
    }
    if (result != ESP_OK) jr_camera_jpeg_release(out);
    camera_release();
    return result;
#endif
}

bool jr_camera_test_once(char *response, size_t capacity) {
    if (!response || capacity < 256) return false;
#if !JR_CAMERA_ENABLED
    snprintf(response, capacity, "JR_ERROR camera_test=disabled");
    return false;
#else
    if (cleanup_failed) {
        snprintf(response, capacity, "JR_ERROR camera_test=cleanup_failed restart_required=1");
        return false;
    }
    jr_camera_jpeg_t frame = {0};
    esp_err_t err = jr_camera_capture_jpeg(&frame);
    if (err != ESP_OK) {
        snprintf(response, capacity, "JR_ERROR camera_test=capture error=%s", esp_err_to_name(err));
        return false;
    }
    unsigned pid = frame.pid;
    unsigned width = frame.width;
    unsigned height = frame.height;
    unsigned bytes = (unsigned)frame.len;
    jr_camera_jpeg_release(&frame);
    snprintf(response, capacity,
             "JR_OK camera_test=frame_received pid=0x%04X width=%u height=%u bytes=%u released=1 optical_check=pending",
             pid, width, height, bytes);
    return true;
#endif
}
