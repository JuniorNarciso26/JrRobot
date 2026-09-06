#include <stdio.h>
#include "jr_board.h"
#include "jr_camera_diag.h"
#if JR_CAMERA_ENABLED
#include "esp_camera.h"
#include "esp_psram.h"
#include "esp_err.h"

/* Do not reinitialize over resources left live by a failed deinit. */
static bool cleanup_failed;
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

bool jr_camera_test_once(char *response, size_t capacity) {
    if (!response || capacity < 256) return false;
#if !JR_CAMERA_ENABLED
    snprintf(response, capacity, "JR_ERROR camera_test=pins_not_confirmed enable_in_menuconfig_after_checking_wiring");
    return false;
#else
    if (cleanup_failed) {
        snprintf(response, capacity, "JR_ERROR camera_test=cleanup_failed restart_required=1");
        return false;
    }
    if (!esp_psram_is_initialized()) {
        snprintf(response, capacity, "JR_ERROR camera_test=psram_unavailable");
        return false;
    }
    esp_err_t err = esp_camera_init(&diagnostic_config);
    if (err != ESP_OK) {
        /* esp32-camera 2.1.6 cleans its own failed initialization. */
        snprintf(response, capacity, "JR_ERROR camera_test=init error=%s", esp_err_to_name(err));
        return false;
    }
    sensor_t *sensor = esp_camera_sensor_get();
    unsigned pid = sensor ? sensor->id.PID : 0;
    camera_fb_t *fb = sensor ? esp_camera_fb_get() : NULL;
    const char *step = !sensor ? "sensor_missing" : !fb ? "frame_timeout" : "invalid_frame";
    unsigned width = 0, height = 0;
    size_t bytes = 0;
    bool valid = fb && fb->buf && fb->len >= 4 && fb->width && fb->height &&
                 fb->format == PIXFORMAT_JPEG && fb->buf[0] == 0xff && fb->buf[1] == 0xd8;
    if (fb) {
        width = (unsigned)fb->width; height = (unsigned)fb->height; bytes = fb->len;
        esp_camera_fb_return(fb);
    }
    err = esp_camera_deinit();
    if (err != ESP_OK) {
        cleanup_failed = true;
        snprintf(response, capacity, "JR_ERROR camera_test=deinit error=%s restart_required=1", esp_err_to_name(err));
        return false;
    }
    if (!valid) {
        snprintf(response, capacity, "JR_ERROR camera_test=%s pid=0x%04X", step, pid);
        return false;
    }
    snprintf(response, capacity,
             "JR_OK camera_test=frame_received pid=0x%04X width=%u height=%u bytes=%u released=1 optical_check=pending",
             pid, width, height, (unsigned)bytes);
    return true;
#endif
}
