#pragma once
#include "esp_err.h"
#include "esp_http_server.h"

/* Connected-mode Wi-Fi manager, served only by the existing HTTPS listener. */
esp_err_t jr_wifi_web_register_routes(httpd_handle_t server);
