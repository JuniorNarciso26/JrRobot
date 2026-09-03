#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t jr_camera_start(void);
bool jr_camera_ready(void);
const char *jr_camera_status_text(void);
esp_err_t jr_camera_capture_handler(httpd_req_t *req);
esp_err_t jr_camera_autofocus_handler(httpd_req_t *req);
esp_err_t jr_camera_manual_focus_handler(httpd_req_t *req);
