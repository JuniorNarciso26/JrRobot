#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

/*
 * Audio WebRTC local integrado ao Live V1.6.3.
 *
 * Registra as rotas HTTPS e usa esp_peer como transporte WebRTC.
 * O caminho de audio usa o I2S full-duplex fisicamente validado no HW04:
 *   BCLK GPIO21, WS GPIO47, RX GPIO41, TX GPIO42
 *   16 kHz, Philips I2S, 2 slots de 32 bits.
 *
 * O transporte WebRTC negocia G.711 A-law (PCMA), 8 kHz mono. A camera continua independente pelo Live JPEG existente.
 */
esp_err_t jr_webrtc_audio_register_routes(httpd_handle_t server);
