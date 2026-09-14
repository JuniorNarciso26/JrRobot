#pragma once
#include <stdbool.h>
#include "esp_err.h"

typedef struct {
    bool saved_record_found;
    bool saved_record_valid;
    char source[16];
    char ssid[33];
    char hostname[33];
    bool static_mode;
    char configured_ip[16];
    char gateway[16];
    char mask[16];
    char dns1[16];
    char dns2[16];
    bool connected;
    bool network_ready;
    bool pending_restart;
    char runtime_ip[16];
    esp_err_t last_error;
} jr_net_diag_t;

void jr_net_diag_snapshot(jr_net_diag_t *out);
