#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t jr_intercom_listen_start_handler(httpd_req_t *req);
esp_err_t jr_intercom_listen_chunk_handler(httpd_req_t *req);
esp_err_t jr_intercom_listen_stop_handler(httpd_req_t *req);
esp_err_t jr_intercom_talk_start_handler(httpd_req_t *req);
esp_err_t jr_intercom_talk_chunk_handler(httpd_req_t *req);
esp_err_t jr_intercom_talk_stop_handler(httpd_req_t *req);
esp_err_t jr_intercom_status_handler(httpd_req_t *req);
