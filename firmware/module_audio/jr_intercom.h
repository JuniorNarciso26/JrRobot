#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t jr_mic_live_start(void);
esp_err_t jr_mic_live_read_pcm16(int16_t *out,size_t max_samples,size_t *samples_read,uint32_t timeout_ms);
void jr_mic_live_stop(void);
bool jr_mic_live_active(void);

esp_err_t jr_audio_talk_start(void);
esp_err_t jr_audio_talk_write_pcm16_mono(const int16_t *samples,size_t sample_count,int sample_rate_hz);
void jr_audio_talk_stop(void);
bool jr_audio_talk_active(void);

esp_err_t jr_intercom_listen_start_handler(httpd_req_t *req);
esp_err_t jr_intercom_listen_chunk_handler(httpd_req_t *req);
esp_err_t jr_intercom_listen_stop_handler(httpd_req_t *req);
esp_err_t jr_intercom_talk_start_handler(httpd_req_t *req);
esp_err_t jr_intercom_talk_chunk_handler(httpd_req_t *req);
esp_err_t jr_intercom_talk_stop_handler(httpd_req_t *req);
esp_err_t jr_intercom_status_handler(httpd_req_t *req);
