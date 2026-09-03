#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_camera.h"
#include "esp_camera_af.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "jr_config.h"
#include "jr_camera.h"

static const char *TAG = "jrbot_camera";

#define AF_SETTLE_MS 350
#define POST_AF_DISCARD_FRAMES 2
#define MANUAL_FOCUS_SETTLE_MS 250
#define OV5640_REG_SYS_RESET00 0x3000
#define OV5640_REG_SYS_CLOCK_ENABLE00 0x3004
#define OV5640_REG_SYS_CLOCK_ENABLE01 0x3005
#define OV5640_REG_VCM_CONTROL_0 0x3602
#define OV5640_REG_VCM_CONTROL_1 0x3603
#define OV5640_REG_VCM_CONTROL_4 0x3606

/* Pinagem validada no teste da câmera ESP32-S3 CAM N16R8 + OV5640.
   IMPORTANTE: GPIO 8 e 9 pertencem ao barramento da câmera nesta placa.
   Por isso o OLED não pode ficar em SDA=8/SCL=9 quando a câmera estiver ligada. */
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  15
#define SIOD_GPIO_NUM  4
#define SIOC_GPIO_NUM  5
#define Y9_GPIO_NUM    16
#define Y8_GPIO_NUM    17
#define Y7_GPIO_NUM    18
#define Y6_GPIO_NUM    12
#define Y5_GPIO_NUM    10
#define Y4_GPIO_NUM    8
#define Y3_GPIO_NUM    9
#define Y2_GPIO_NUM    11
#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM  7
#define PCLK_GPIO_NUM  13

static SemaphoreHandle_t camera_lock;
static bool camera_ok;
static bool autofocus_initialized;
static int last_manual_focus_percent = -1;
static uint16_t last_manual_focus_vcm;
static char status_text[160] = "camera nao iniciada";

static const camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sccb_sda = SIOD_GPIO_NUM,
    .pin_sccb_scl = SIOC_GPIO_NUM,
    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_SVGA,
    .jpeg_quality = 12,
    .fb_count = 1,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

static bool log_error(const char *step, esp_err_t err) {
    if (err == ESP_OK) return false;
    ESP_LOGE(TAG, "%s falhou: %s (0x%" PRIx32 ")", step, esp_err_to_name(err), (uint32_t)err);
    return true;
}

static esp_err_t restore_capture_configuration(sensor_t *sensor) {
    if (!sensor || !sensor->set_pixformat || !sensor->set_framesize || !sensor->set_quality) return ESP_ERR_NOT_SUPPORTED;
    esp_err_t err = (esp_err_t)sensor->set_pixformat(sensor, PIXFORMAT_JPEG);
    if (err != ESP_OK) return err;
    err = (esp_err_t)sensor->set_framesize(sensor, FRAMESIZE_SVGA);
    if (err != ESP_OK) return err;
    return (esp_err_t)sensor->set_quality(sensor, 12);
}

static esp_err_t run_autofocus(sensor_t *sensor) {
    if (!sensor || !esp_camera_af_is_supported(sensor)) return ESP_ERR_NOT_SUPPORTED;
    if (!autofocus_initialized) {
        const esp_camera_af_config_t af_config = {.mode = ESP_CAMERA_AF_MODE_MANUAL, .timeout_ms = CONFIG_CAMERA_AF_DEFAULT_TIMEOUT_MS};
        esp_err_t err = esp_camera_af_init(sensor, &af_config);
        if (err != ESP_OK) return err;
        autofocus_initialized = true;
    }
    esp_err_t err = esp_camera_af_set_mode(sensor, ESP_CAMERA_AF_MODE_MANUAL);
    if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(100));
    err = esp_camera_af_trigger(sensor);
    if (err != ESP_OK) return err;
    esp_camera_af_status_t status = {0};
    err = esp_camera_af_wait(sensor, CONFIG_CAMERA_AF_DEFAULT_TIMEOUT_MS, &status);
    if (err != ESP_OK) return err;
    return status.focused ? ESP_OK : ESP_FAIL;
}

static void discard_camera_frames(int count) {
    for (int i = 0; i < count; ++i) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(80));
    }
}

static bool request_wants_focus(httpd_req_t *req) {
    char query[64] = {0};
    char value[8] = {0};
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) return false;
    if (httpd_query_key_value(query, "focus", value, sizeof(value)) != ESP_OK) return false;
    return !strcmp(value, "1") || !strcmp(value, "true") || !strcmp(value, "sim");
}

static esp_err_t get_query_int(httpd_req_t *req, const char *key, int *out) {
    char query[96] = {0};
    char value[16] = {0};
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) return ESP_ERR_INVALID_ARG;
    if (httpd_query_key_value(query, key, value, sizeof(value)) != ESP_OK) return ESP_ERR_INVALID_ARG;
    char *end = NULL;
    long parsed = strtol(value, &end, 10);
    if (!end || *end != '\0') return ESP_ERR_INVALID_ARG;
    *out = (int)parsed;
    return ESP_OK;
}

static esp_err_t set_manual_focus_percent(sensor_t *sensor, int percent) {
    if (!sensor) return ESP_ERR_INVALID_STATE;
    if (!autofocus_initialized) {
        const esp_camera_af_config_t af_config = {.mode = ESP_CAMERA_AF_MODE_MANUAL, .timeout_ms = CONFIG_CAMERA_AF_DEFAULT_TIMEOUT_MS};
        esp_err_t err = esp_camera_af_init(sensor, &af_config);
        if (err != ESP_OK) return err;
        autofocus_initialized = true;
    }
    esp_err_t err = esp_camera_af_set_mode(sensor, ESP_CAMERA_AF_MODE_MANUAL);
    if (err != ESP_OK) return err;
    if (!sensor->set_reg) return ESP_ERR_NOT_SUPPORTED;
    uint16_t vcm = (uint16_t)((percent * 1023) / 100);
    int ret = sensor->set_reg(sensor, OV5640_REG_SYS_CLOCK_ENABLE00, 0x60, 0x60);
    if (ret) return ESP_FAIL;
    ret = sensor->set_reg(sensor, OV5640_REG_SYS_CLOCK_ENABLE01, 0x40, 0x40);
    if (ret) return ESP_FAIL;
    ret = sensor->set_reg(sensor, OV5640_REG_VCM_CONTROL_1, 0x80, 0x00);
    if (ret) return ESP_FAIL;
    ret = sensor->set_reg(sensor, OV5640_REG_VCM_CONTROL_4, 0xFF, 0x3F);
    if (ret) return ESP_FAIL;
    ret = sensor->set_reg(sensor, OV5640_REG_VCM_CONTROL_0, 0xF0, (vcm & 0x0F) << 4);
    if (ret) return ESP_FAIL;
    ret = sensor->set_reg(sensor, OV5640_REG_VCM_CONTROL_1, 0x3F, (vcm >> 4) & 0x3F);
    if (ret) return ESP_FAIL;
    ret = sensor->set_reg(sensor, OV5640_REG_VCM_CONTROL_0, 0x07, 0x00);
    if (ret) return ESP_FAIL;
    last_manual_focus_percent = percent;
    last_manual_focus_vcm = vcm;
    vTaskDelay(pdMS_TO_TICKS(MANUAL_FOCUS_SETTLE_MS));
    return restore_capture_configuration(sensor);
}

static esp_err_t send_plain_error(httpd_req_t *req, httpd_err_code_t status, const char *message, esp_err_t err) {
    ESP_LOGE(TAG, "%s: %s (0x%" PRIx32 ")", message, esp_err_to_name(err), (uint32_t)err);
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    return httpd_resp_send_err(req, status, message);
}

bool jr_camera_ready(void) { return camera_ok; }
const char *jr_camera_status_text(void) { return status_text; }

esp_err_t jr_camera_start(void) {
    camera_lock = xSemaphoreCreateMutex();
    if (!camera_lock) return ESP_ERR_NO_MEM;
    if (!esp_psram_is_initialized()) {
        esp_err_t err = esp_psram_init();
        if (err != ESP_OK) {
            snprintf(status_text, sizeof(status_text), "PSRAM falhou: %s", esp_err_to_name(err));
            return err;
        }
    }
    esp_err_t err = esp_camera_init(&camera_config);
    if (log_error("camera init", err)) {
        snprintf(status_text, sizeof(status_text), "camera init falhou: %s", esp_err_to_name(err));
        return err;
    }
    sensor_t *sensor = esp_camera_sensor_get();
    if (!sensor) return ESP_ERR_INVALID_STATE;
    if (sensor->id.PID != OV5640_PID) {
        snprintf(status_text, sizeof(status_text), "sensor PID 0x%04x (esperado OV5640)", sensor->id.PID);
        ESP_LOGW(TAG, "%s", status_text);
    } else {
        snprintf(status_text, sizeof(status_text), "OV5640 pronta PID=0x%04x", sensor->id.PID);
    }
    restore_capture_configuration(sensor);
    discard_camera_frames(2);
    camera_ok = true;
    ESP_LOGI(TAG, "Camera pronta: %s", status_text);
    return ESP_OK;
}

esp_err_t jr_camera_autofocus_handler(httpd_req_t *req) {
    if (!camera_ok) return send_plain_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Camera nao iniciada.", ESP_ERR_INVALID_STATE);
    if (xSemaphoreTake(camera_lock, pdMS_TO_TICKS(8000)) != pdTRUE) return send_plain_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Camera ocupada.", ESP_ERR_TIMEOUT);
    sensor_t *sensor = esp_camera_sensor_get();
    esp_err_t err = run_autofocus(sensor);
    if (err == ESP_OK) err = restore_capture_configuration(sensor);
    xSemaphoreGive(camera_lock);
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    if (err != ESP_OK) return send_plain_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Nao foi possivel ajustar o foco.", err);
    return httpd_resp_sendstr(req, "Foco ajustado com sucesso.");
}

esp_err_t jr_camera_manual_focus_handler(httpd_req_t *req) {
    int percent = 0;
    esp_err_t err = get_query_int(req, "value", &percent);
    if (err != ESP_OK || percent < 0 || percent > 100) return send_plain_error(req, HTTPD_400_BAD_REQUEST, "Valor de foco invalido. Use 0 a 100.", ESP_ERR_INVALID_ARG);
    if (!camera_ok) return send_plain_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Camera nao iniciada.", ESP_ERR_INVALID_STATE);
    if (xSemaphoreTake(camera_lock, pdMS_TO_TICKS(8000)) != pdTRUE) return send_plain_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Camera ocupada.", ESP_ERR_TIMEOUT);
    sensor_t *sensor = esp_camera_sensor_get();
    err = set_manual_focus_percent(sensor, percent);
    xSemaphoreGive(camera_lock);
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    if (err != ESP_OK) return send_plain_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Nao foi possivel aplicar foco manual.", err);
    char response[120] = {0};
    snprintf(response, sizeof(response), "Foco manual aplicado: %d%% (VCM=%u).", last_manual_focus_percent, last_manual_focus_vcm);
    return httpd_resp_sendstr(req, response);
}

esp_err_t jr_camera_capture_handler(httpd_req_t *req) {
    if (!camera_ok) return send_plain_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Camera nao iniciada.", ESP_ERR_INVALID_STATE);
    if (xSemaphoreTake(camera_lock, pdMS_TO_TICKS(12000)) != pdTRUE) return send_plain_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Camera ocupada.", ESP_ERR_TIMEOUT);
    sensor_t *sensor = esp_camera_sensor_get();
    esp_err_t err = ESP_OK;
    if (!sensor) err = ESP_ERR_INVALID_STATE;
    else if (request_wants_focus(req)) {
        err = run_autofocus(sensor);
        if (err == ESP_OK) err = restore_capture_configuration(sensor);
        if (err == ESP_OK) discard_camera_frames(POST_AF_DISCARD_FRAMES);
    }
    if (err != ESP_OK) {
        xSemaphoreGive(camera_lock);
        return send_plain_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Falha no foco antes da captura.", err);
    }
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        xSemaphoreGive(camera_lock);
        return send_plain_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Erro ao capturar imagem.", ESP_FAIL);
    }
    ESP_LOGI(TAG, "JPEG capturado: %ux%u, %u bytes", fb->width, fb->height, (unsigned)fb->len);
    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    err = httpd_resp_send(req, (const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);
    xSemaphoreGive(camera_lock);
    return err;
}
