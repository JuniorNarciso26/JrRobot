#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "jr_mode_manager.h"
#include "jr_wifi.h"
#include "jr_wifi_provisioning.h"

#define JR_SETUP_MAGIC 0x4a525750u
#define JR_SETUP_BUTTON_MS 3000u
#define JR_SETUP_FORM_MAX 512
#define JR_SETUP_SCAN_LIMIT 16
#define JR_SETUP_TEST_TIMEOUT_MS 20000u
#define JR_SETUP_RESTART_DELAY_MS 10000u

typedef enum {
    JR_SETUP_TEST_IDLE,
    JR_SETUP_TEST_CONNECTING,
    JR_SETUP_TEST_GOT_IP,
    JR_SETUP_TEST_SAVING,
    JR_SETUP_TEST_SAVED,
    JR_SETUP_TEST_BAD_PASSWORD,
    JR_SETUP_TEST_NO_NETWORK,
    JR_SETUP_TEST_NO_IP,
    JR_SETUP_TEST_TIMEOUT,
    JR_SETUP_TEST_FAILED,
    JR_SETUP_TEST_SAVE_FAILED,
} jr_setup_test_state_t;

typedef struct {
    char ssid[33];
    char password[65];
} jr_setup_candidate_t;

static const char *TAG = "jrbot_wifi_setup";
static RTC_NOINIT_ATTR uint32_t setup_request;
static RTC_NOINIT_ATTR uint32_t setup_request_inverse;
static esp_netif_t *setup_ap;
static esp_netif_t *setup_sta;
static httpd_handle_t setup_http;
static esp_event_handler_instance_t setup_wifi_handler;
static esp_event_handler_instance_t setup_ip_handler;
static bool setup_wifi_handler_set;
static bool setup_ip_handler_set;
static int setup_dns_socket = -1;
static uint32_t setup_dns_ip;
static bool setup_running;
static bool button_ready;
static bool button_armed;
static uint32_t button_down_ms;
static portMUX_TYPE setup_test_lock = portMUX_INITIALIZER_UNLOCKED;
static jr_setup_candidate_t *setup_candidate;
static jr_setup_test_state_t setup_test_state;
static uint32_t setup_test_started_ms;
static uint32_t setup_restart_ms;
static uint16_t setup_test_reason;
static bool setup_test_linked;
static bool setup_test_cleanup_busy;
static char setup_test_ip[16];
extern const char setup_page[] asm("_binary_jr_wifi_setup_html_start");

static void setup_memory(const char *stage) {
    ESP_LOGI(TAG, "JR_WIFI_SETUP_MEMORY stage=%s internal_free=%u internal_largest=%u psram_free=%u",
             stage,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

bool jr_wifi_provisioning_boot_requested(void) {
    bool requested = esp_reset_reason() == ESP_RST_SW &&
                     setup_request == JR_SETUP_MAGIC &&
                     setup_request_inverse == ~JR_SETUP_MAGIC;
    setup_request = 0;
    setup_request_inverse = 0;
    return requested || !jr_wifi_is_configured();
}

static int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool decode_value(char *out, size_t capacity, const char *encoded) {
    size_t used = 0;
    for (size_t i = 0; encoded[i]; i++) {
        unsigned char ch = (unsigned char)encoded[i];
        if (ch == '+') ch = ' ';
        else if (ch == '%') {
            int hi = hex_value(encoded[i + 1]);
            int lo = hi < 0 || !encoded[i + 1] ? -1 : hex_value(encoded[i + 2]);
            if (hi < 0 || lo < 0) return false;
            ch = (unsigned char)((hi << 4) | lo);
            i += 2;
        }
        if (ch < 32 || ch == 127 || used + 1 >= capacity) return false;
        out[used++] = (char)ch;
    }
    out[used] = 0;
    return true;
}

static bool parse_form(char *form, char *ssid, char *password) {
    bool have_ssid = false, have_password = false;
    for (char *part = form; part && *part;) {
        char *next = strchr(part, '&');
        if (next) *next++ = 0;
        char *value = strchr(part, '=');
        if (!value) return false;
        *value++ = 0;
        if (!strcmp(part, "ssid")) {
            if (have_ssid || !decode_value(ssid, 33, value)) return false;
            have_ssid = true;
        } else if (!strcmp(part, "pass")) {
            if (have_password || !decode_value(password, 65, value)) return false;
            have_password = true;
        }
        part = next;
    }
    return have_ssid && have_password && ssid[0];
}

static uint32_t setup_now_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static bool setup_test_busy(void) {
    portENTER_CRITICAL(&setup_test_lock);
    bool busy = setup_candidate || setup_test_cleanup_busy ||
                setup_test_state == JR_SETUP_TEST_SAVING ||
                setup_test_state == JR_SETUP_TEST_SAVED;
    portEXIT_CRITICAL(&setup_test_lock);
    return busy;
}

static void setup_test_event(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_CONNECTED) {
        portENTER_CRITICAL(&setup_test_lock);
        if (setup_test_state == JR_SETUP_TEST_CONNECTING) setup_test_linked = true;
        portEXIT_CRITICAL(&setup_test_lock);
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        const wifi_event_sta_disconnected_t *event = data;
        portENTER_CRITICAL(&setup_test_lock);
        if (setup_test_state == JR_SETUP_TEST_CONNECTING ||
            setup_test_state == JR_SETUP_TEST_GOT_IP) {
            setup_test_reason = event->reason;
            if (event->reason == WIFI_REASON_AUTH_FAIL ||
                event->reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT ||
                event->reason == WIFI_REASON_HANDSHAKE_TIMEOUT)
                setup_test_state = JR_SETUP_TEST_BAD_PASSWORD;
            else if (event->reason == WIFI_REASON_NO_AP_FOUND ||
                     event->reason == WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY ||
                     event->reason == WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD)
                setup_test_state = JR_SETUP_TEST_NO_NETWORK;
            else setup_test_state = JR_SETUP_TEST_FAILED;
        }
        portEXIT_CRITICAL(&setup_test_lock);
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *event = data;
        char ip[16];
        snprintf(ip, sizeof(ip), IPSTR, IP2STR(&event->ip_info.ip));
        portENTER_CRITICAL(&setup_test_lock);
        if (setup_test_state == JR_SETUP_TEST_CONNECTING) {
            memcpy(setup_test_ip, ip, sizeof(ip));
            setup_test_state = JR_SETUP_TEST_GOT_IP;
        }
        portEXIT_CRITICAL(&setup_test_lock);
    }
}

static const char *setup_test_name(jr_setup_test_state_t state) {
    switch (state) {
    case JR_SETUP_TEST_CONNECTING:
    case JR_SETUP_TEST_GOT_IP:
    case JR_SETUP_TEST_SAVING: return "testing";
    case JR_SETUP_TEST_SAVED: return "saved";
    case JR_SETUP_TEST_BAD_PASSWORD: return "bad_password";
    case JR_SETUP_TEST_NO_NETWORK: return "no_network";
    case JR_SETUP_TEST_NO_IP: return "no_ip";
    case JR_SETUP_TEST_TIMEOUT: return "timeout";
    case JR_SETUP_TEST_SAVE_FAILED: return "save_failed";
    case JR_SETUP_TEST_FAILED: return "failed";
    default: return "idle";
    }
}

static void setup_test_poll(void) {
    jr_setup_candidate_t *candidate = NULL;
    jr_setup_test_state_t state;
    bool restart = false;
    uint32_t now = setup_now_ms();
    portENTER_CRITICAL(&setup_test_lock);
    if (setup_test_state == JR_SETUP_TEST_CONNECTING &&
        (uint32_t)(now - setup_test_started_ms) >= JR_SETUP_TEST_TIMEOUT_MS)
        setup_test_state = setup_test_linked ? JR_SETUP_TEST_NO_IP : JR_SETUP_TEST_TIMEOUT;
    if (setup_test_state == JR_SETUP_TEST_GOT_IP) {
        setup_test_state = JR_SETUP_TEST_SAVING;
        candidate = setup_candidate;
        setup_candidate = NULL;
    } else if (setup_test_state >= JR_SETUP_TEST_BAD_PASSWORD && setup_candidate) {
        candidate = setup_candidate;
        setup_candidate = NULL;
        setup_test_cleanup_busy = true;
    }
    state = setup_test_state;
    if (state == JR_SETUP_TEST_SAVED &&
        (uint32_t)(now - setup_restart_ms) >= JR_SETUP_RESTART_DELAY_MS)
        restart = true;
    portEXIT_CRITICAL(&setup_test_lock);

    if (state == JR_SETUP_TEST_SAVING && candidate) {
        wifi_ap_record_t ap;
        bool connected = esp_wifi_sta_get_ap_info(&ap) == ESP_OK;
        char result[160] = {0};
        bool saved = connected && jr_wifi_configure(candidate->ssid, candidate->password,
                                                     "jrbot", false, NULL, NULL, NULL,
                                                     NULL, NULL, result, sizeof(result));
        memset(candidate, 0, sizeof(*candidate));
        free(candidate);
        if (!saved) {
            (void)esp_wifi_disconnect();
            wifi_config_t empty = {0};
            (void)esp_wifi_set_config(WIFI_IF_STA, &empty);
        }
        portENTER_CRITICAL(&setup_test_lock);
        setup_test_state = saved ? JR_SETUP_TEST_SAVED :
                           connected ? JR_SETUP_TEST_SAVE_FAILED : JR_SETUP_TEST_FAILED;
        setup_restart_ms = now;
        portEXIT_CRITICAL(&setup_test_lock);
        ESP_LOGI(TAG, "JR_WIFI_SETUP_TEST result=%s ip=%s",
                 saved ? "saved" : connected ? "save_failed" : "disconnected",
                 saved ? setup_test_ip : "-");
    } else if (candidate) {
        ESP_LOGW(TAG, "JR_WIFI_SETUP_TEST result=%s reason=%u no_save=1",
                 setup_test_name(state), (unsigned)setup_test_reason);
        (void)esp_wifi_disconnect();
        wifi_config_t empty = {0};
        (void)esp_wifi_set_config(WIFI_IF_STA, &empty);
        memset(candidate, 0, sizeof(*candidate));
        free(candidate);
        portENTER_CRITICAL(&setup_test_lock);
        setup_test_cleanup_busy = false;
        portEXIT_CRITICAL(&setup_test_lock);
    }
    if (restart) esp_restart();
}

static esp_err_t setup_index(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, setup_page);
}

static void setup_json_ssid(const uint8_t *ssid, char *encoded, size_t capacity) {
    size_t used = 0;
    for (size_t i = 0; i < 32 && ssid[i] && used + 7 < capacity; i++) {
        unsigned char ch = ssid[i];
        if (ch == '"' || ch == '\\') { encoded[used++] = '\\'; encoded[used++] = ch; }
        else if (ch < 32) {
            int count = snprintf(encoded + used, capacity - used, "\\u%04X", ch);
            used += count > 0 ? (size_t)count : 0;
        } else encoded[used++] = ch;
    }
    encoded[used] = 0;
}

static esp_err_t setup_scan_error(httpd_req_t *req, const char *message) {
    httpd_resp_set_status(req, "503 Service Unavailable");
    return httpd_resp_sendstr(req, message);
}

static esp_err_t setup_scan(httpd_req_t *req) {
    /* Scan only when the page asks for it; no timer or scan cache in RAM. */
    if (setup_test_busy()) return setup_scan_error(req, "Teste de conexao em andamento");
    wifi_scan_config_t scan = {.show_hidden = false,
                               .scan_type = WIFI_SCAN_TYPE_ACTIVE,
                               .scan_time.active = {.min = 0, .max = 90}};
    esp_err_t err = esp_wifi_scan_start(&scan, true);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "JR_WIFI_SETUP_SCAN result=%s", esp_err_to_name(err));
        return setup_scan_error(req, "Busca indisponivel");
    }
    uint16_t count = 0;
    err = esp_wifi_scan_get_ap_num(&count);
    if (err != ESP_OK) {
        (void)esp_wifi_clear_ap_list();
        return setup_scan_error(req, "Busca indisponivel");
    }
    uint16_t shown = count < JR_SETUP_SCAN_LIMIT ? count : JR_SETUP_SCAN_LIMIT;
    wifi_ap_record_t *records = shown ? heap_caps_calloc(shown, sizeof(*records),
                                                          MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : NULL;
    if (shown && !records) {
        (void)esp_wifi_clear_ap_list();
        return setup_scan_error(req, "Memoria insuficiente");
    }
    if (shown) err = esp_wifi_scan_get_ap_records(&shown, records);
    else (void)esp_wifi_clear_ap_list();
    if (err != ESP_OK) {
        (void)esp_wifi_clear_ap_list();
        free(records);
        return setup_scan_error(req, "Busca indisponivel");
    }
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    err = httpd_resp_send_chunk(req, "{\"aps\":[", 8);
    for (uint16_t i = 0; i < shown && err == ESP_OK; i++) {
        char encoded[193];
        char item[255];
        setup_json_ssid(records[i].ssid, encoded, sizeof(encoded));
        int length = snprintf(item, sizeof(item),
                              "%s{\"ssid\":\"%s\",\"rssi\":%d,\"open\":%s}",
                              i ? "," : "", encoded, records[i].rssi,
                              records[i].authmode == WIFI_AUTH_OPEN ? "true" : "false");
        if (length < 0 || (size_t)length >= sizeof(item)) { err = ESP_ERR_INVALID_SIZE; break; }
        err = httpd_resp_send_chunk(req, item, length);
    }
    free(records);
    if (err != ESP_OK) return err;
    err = httpd_resp_send_chunk(req, "]}", 2);
    return err == ESP_OK ? httpd_resp_send_chunk(req, NULL, 0) : err;
}

static esp_err_t setup_profiles(httpd_req_t *req) {
    char *response = heap_caps_malloc(1024, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!response) return setup_scan_error(req, "Memoria insuficiente");
    bool ok = jr_wifi_profiles_list(response, 1024);
    if (!ok) {
        ESP_LOGW(TAG, "JR_WIFI_SETUP_PROFILES result=%s", response);
        free(response);
        return setup_scan_error(req, "Consulta indisponivel");
    }
    const char prefix[] = "JR_WIFI_PROFILES ";
    if (strncmp(response, prefix, sizeof(prefix) - 1)) {
        free(response);
        return setup_scan_error(req, "Resposta invalida");
    }
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    esp_err_t err = httpd_resp_sendstr(req, response + sizeof(prefix) - 1);
    free(response);
    return err;
}

static esp_err_t setup_profile_delete(httpd_req_t *req) {
    if (setup_test_busy()) return setup_scan_error(req, "Teste de conexao em andamento");
    char query[24], id_text[8], *end;
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
        httpd_query_key_value(query, "id", id_text, sizeof(id_text)) != ESP_OK ||
        !id_text[0])
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "ID invalido");
    unsigned long id = strtoul(id_text, &end, 10);
    if (*end || id > 3)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "ID invalido");
    char response[160];
    if (!jr_wifi_profile_delete((unsigned)id, response, sizeof(response))) {
        ESP_LOGW(TAG, "JR_WIFI_SETUP_DELETE result=%s", response);
        return setup_scan_error(req, "Nao foi possivel apagar a rede");
    }
    ESP_LOGI(TAG, "JR_WIFI_SETUP_DELETE id=%lu", id);
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, "{\"result\":\"ok\"}");
}

static esp_err_t setup_not_found(httpd_req_t *req, httpd_err_code_t error) {
    (void)error;
    /* Phone connectivity probes need an HTTP response with content. */
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, "<a href='http://192.168.4.1/'>Configurar Wi-Fi do JrBot</a>");
}

/* Answer every IPv4 DNS query with the AP address while setup is active.
 * Polling on app_main avoids a dedicated DNS task and its 4 KB stack. */
static size_t setup_dns_reply(uint8_t *packet, size_t length) {
    if (length < 17 || (packet[2] & 0xf8) || packet[4] != 0 || packet[5] != 1)
        return 0;
    size_t offset = 12;
    bool end_of_name = false;
    while (offset < length) {
        uint8_t label = packet[offset++];
        if (!label) { end_of_name = true; break; }
        if (label > 63 || offset + label > length || offset + label > 255) return 0;
        offset += label;
    }
    if (!end_of_name || offset + 4 > length) return 0;
    bool address_query = packet[offset] == 0 && packet[offset + 1] == 1 &&
                         packet[offset + 2] == 0 && packet[offset + 3] == 1;
    size_t reply_length = offset + 4;
    if (address_query && reply_length + 16 > 512) return 0;
    packet[2] = 0x81; /* response, recursion desired/available */
    packet[3] = 0x80;
    packet[6] = 0;
    packet[7] = address_query ? 1 : 0;
    packet[8] = packet[9] = packet[10] = packet[11] = 0;
    if (address_query) {
        static const uint8_t answer[] = {
            0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
        };
        memcpy(packet + reply_length, answer, sizeof(answer));
        memcpy(packet + reply_length + sizeof(answer), &setup_dns_ip, 4);
        reply_length += 16;
    }
    return reply_length;
}

void jr_wifi_provisioning_poll(void) {
    if (setup_running) setup_test_poll();
    if (setup_dns_socket < 0) return;
    for (int i = 0; i < 4; i++) {
        uint8_t packet[512];
        struct sockaddr_in peer = {0};
        socklen_t peer_length = sizeof(peer);
        int count = recvfrom(setup_dns_socket, packet, sizeof(packet), MSG_DONTWAIT,
                             (struct sockaddr *)&peer, &peer_length);
        if (count < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
                ESP_LOGW(TAG, "JR_WIFI_SETUP_DNS recv_errno=%d", errno);
            return;
        }
        size_t reply_length = setup_dns_reply(packet, (size_t)count);
        if (reply_length)
            (void)sendto(setup_dns_socket, packet, reply_length, 0,
                         (struct sockaddr *)&peer, peer_length);
    }
}

static void setup_dns_start(void) {
    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(53),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    setup_dns_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (setup_dns_socket >= 0 &&
        bind(setup_dns_socket, (struct sockaddr *)&address, sizeof(address)) == 0)
        return;
    ESP_LOGW(TAG, "JR_WIFI_SETUP_DNS unavailable errno=%d; use 192.168.4.1", errno);
    if (setup_dns_socket >= 0) close(setup_dns_socket);
    setup_dns_socket = -1;
}

static esp_err_t setup_test_status(httpd_req_t *req) {
    jr_setup_test_state_t state;
    uint16_t reason;
    char ip[16];
    portENTER_CRITICAL(&setup_test_lock);
    state = setup_test_state;
    if (state >= JR_SETUP_TEST_BAD_PASSWORD &&
        (setup_candidate || setup_test_cleanup_busy))
        state = JR_SETUP_TEST_CONNECTING;
    reason = setup_test_reason;
    memcpy(ip, setup_test_ip, sizeof(ip));
    portEXIT_CRITICAL(&setup_test_lock);
    char json[128];
    snprintf(json, sizeof(json), "{\"state\":\"%s\",\"reason\":%u,\"ip\":\"%s\"}",
             setup_test_name(state), (unsigned)reason, ip);
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, json);
}

static esp_err_t setup_test_start(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    if (setup_test_busy()) {
        httpd_resp_set_status(req, "409 Conflict");
        return httpd_resp_sendstr(req, "{\"state\":\"busy\"}");
    }
    if (req->content_len <= 0 || req->content_len > JR_SETUP_FORM_MAX) {
        httpd_resp_set_status(req, "400 Bad Request");
        return httpd_resp_sendstr(req, "{\"state\":\"invalid\"}");
    }
    char form[JR_SETUP_FORM_MAX + 1];
    size_t used = 0;
    while (used < (size_t)req->content_len) {
        int count = httpd_req_recv(req, form + used, (size_t)req->content_len - used);
        if (count <= 0) {
            memset(form, 0, sizeof(form));
            httpd_resp_set_status(req, "400 Bad Request");
            return httpd_resp_sendstr(req, "{\"state\":\"invalid\"}");
        }
        used += (size_t)count;
    }
    form[used] = 0;
    char ssid[33] = {0}, password[65] = {0};
    bool valid = parse_form(form, ssid, password);
    size_t pass_len = strlen(password);
    if (pass_len && pass_len < 8) valid = false;
    if (pass_len == 64)
        for (size_t i = 0; i < pass_len; i++)
            if (!isxdigit((unsigned char)password[i])) valid = false;
    if (!valid) {
        memset(password, 0, sizeof(password));
        memset(form, 0, sizeof(form));
        httpd_resp_set_status(req, "400 Bad Request");
        return httpd_resp_sendstr(req, "{\"state\":\"invalid\"}");
    }
    jr_setup_candidate_t *candidate = heap_caps_calloc(1, sizeof(*candidate),
                                                       MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!candidate) {
        memset(password, 0, sizeof(password));
        memset(form, 0, sizeof(form));
        return setup_scan_error(req, "Memoria insuficiente");
    }
    memcpy(candidate->ssid, ssid, strlen(ssid) + 1);
    memcpy(candidate->password, password, pass_len + 1);
    memset(password, 0, sizeof(password));
    memset(form, 0, sizeof(form));

    portENTER_CRITICAL(&setup_test_lock);
    setup_candidate = candidate;
    setup_test_state = JR_SETUP_TEST_CONNECTING;
    setup_test_started_ms = setup_now_ms();
    setup_test_reason = 0;
    setup_test_linked = false;
    setup_test_ip[0] = 0;
    portEXIT_CRITICAL(&setup_test_lock);

    wifi_config_t config = {0};
    memcpy(config.sta.ssid, candidate->ssid, strlen(candidate->ssid));
    memcpy(config.sta.password, candidate->password, strlen(candidate->password));
    config.sta.threshold.authmode = candidate->password[0] ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &config);
    if (err == ESP_OK) err = esp_wifi_connect();
    memset(&config, 0, sizeof(config));
    if (err != ESP_OK) {
        portENTER_CRITICAL(&setup_test_lock);
        setup_test_state = JR_SETUP_TEST_FAILED;
        portEXIT_CRITICAL(&setup_test_lock);
        ESP_LOGW(TAG, "JR_WIFI_SETUP_TEST start=%s no_save=1", esp_err_to_name(err));
        return setup_scan_error(req, "Nao foi possivel iniciar o teste");
    }
    ESP_LOGI(TAG, "JR_WIFI_SETUP_TEST state=connecting ssid=%s", ssid);
    httpd_resp_set_status(req, "202 Accepted");
    return httpd_resp_sendstr(req, "{\"state\":\"testing\"}");
}

esp_err_t jr_wifi_provisioning_start(void) {
    if (setup_running) return ESP_OK;
    setup_memory("before");
    jr_wifi_stop();
    setup_memory("after_sta_stop");
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) goto fail;
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) goto fail;

    esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_WIFI_AP();
    setup_ap = esp_netif_new(&netif_config);
    if (!setup_ap) { err = ESP_ERR_NO_MEM; goto fail; }
    err = esp_netif_attach_wifi_ap(setup_ap);
    if (err != ESP_OK) goto fail;
    err = esp_wifi_set_default_wifi_ap_handlers();
    if (err != ESP_OK) goto fail;
    esp_netif_ip_info_t ap_ip = {0};
    err = esp_netif_get_ip_info(setup_ap, &ap_ip);
    if (err != ESP_OK) goto fail;
    setup_dns_ip = ap_ip.ip.addr;
    esp_netif_dns_info_t dns = {.ip.type = ESP_IPADDR_TYPE_V4};
    dns.ip.u_addr.ip4.addr = setup_dns_ip;
    err = esp_netif_set_dns_info(setup_ap, ESP_NETIF_DNS_MAIN, &dns);
    if (err != ESP_OK) goto fail;

    esp_netif_config_t sta_netif_config = ESP_NETIF_DEFAULT_WIFI_STA();
    setup_sta = esp_netif_new(&sta_netif_config);
    if (!setup_sta) { err = ESP_ERR_NO_MEM; goto fail; }
    err = esp_netif_attach_wifi_station(setup_sta);
    if (err != ESP_OK) goto fail;
    err = esp_wifi_set_default_wifi_sta_handlers();
    if (err != ESP_OK) goto fail;
    err = esp_netif_set_hostname(setup_sta, "jrbot");
    if (err != ESP_OK) goto fail;

    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&init);
    if (err != ESP_OK) goto fail;
    err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (err != ESP_OK) goto fail_driver;
    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                               setup_test_event, NULL, &setup_wifi_handler);
    if (err != ESP_OK) goto fail_driver;
    setup_wifi_handler_set = true;
    err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               setup_test_event, NULL, &setup_ip_handler);
    if (err != ESP_OK) goto fail_driver;
    setup_ip_handler_set = true;

    uint8_t mac[6] = {0};
    (void)esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    wifi_config_t config = {0};
    snprintf((char *)config.ap.ssid, sizeof(config.ap.ssid), "JrBot-Setup-%02X%02X%02X",
             mac[3], mac[4], mac[5]);
    config.ap.ssid_len = strlen((const char *)config.ap.ssid);
    config.ap.authmode = WIFI_AUTH_OPEN;
    config.ap.max_connection = 2;
    config.ap.channel = 1;
    /* AP remains available while STA scans and validates the selected network. */
    err = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (err != ESP_OK) goto fail_driver;
    err = esp_wifi_set_config(WIFI_IF_AP, &config);
    if (err != ESP_OK) goto fail_driver;
    wifi_config_t empty_sta = {0};
    err = esp_wifi_set_config(WIFI_IF_STA, &empty_sta);
    if (err != ESP_OK) goto fail_driver;
    err = esp_wifi_start();
    if (err != ESP_OK) goto fail_driver;

    httpd_config_t http_config = HTTPD_DEFAULT_CONFIG();
    http_config.max_uri_handlers = 6;
    http_config.lru_purge_enable = true;
    err = httpd_start(&setup_http, &http_config);
    if (err != ESP_OK) goto fail_started;
    httpd_uri_t index = {.uri = "/", .method = HTTP_GET, .handler = setup_index};
    httpd_uri_t test_start = {.uri = "/test", .method = HTTP_POST, .handler = setup_test_start};
    httpd_uri_t test_status = {.uri = "/test/status", .method = HTTP_GET, .handler = setup_test_status};
    httpd_uri_t scan_route = {.uri = "/scan", .method = HTTP_GET, .handler = setup_scan};
    httpd_uri_t profiles = {.uri = "/profiles", .method = HTTP_GET, .handler = setup_profiles};
    httpd_uri_t profile_delete = {.uri = "/profiles", .method = HTTP_DELETE, .handler = setup_profile_delete};
    err = httpd_register_uri_handler(setup_http, &index);
    if (err == ESP_OK) err = httpd_register_uri_handler(setup_http, &test_start);
    if (err == ESP_OK) err = httpd_register_uri_handler(setup_http, &test_status);
    if (err == ESP_OK) err = httpd_register_uri_handler(setup_http, &scan_route);
    if (err == ESP_OK) err = httpd_register_uri_handler(setup_http, &profiles);
    if (err == ESP_OK) err = httpd_register_uri_handler(setup_http, &profile_delete);
    if (err == ESP_OK)
        err = httpd_register_err_handler(setup_http, HTTPD_404_NOT_FOUND, setup_not_found);
    if (err != ESP_OK) {
        httpd_stop(setup_http);
        setup_http = NULL;
        goto fail_started;
    }
    setup_dns_start();
    setup_running = true;
    ESP_LOGI(TAG, "JR_WIFI_SETUP_READY ssid=%s security=open url=http://192.168.4.1 captive_dns=%s",
             config.ap.ssid, setup_dns_socket >= 0 ? "on" : "off");
    setup_memory("ready");
    return ESP_OK;

fail_started:
    esp_wifi_stop();
fail_driver:
    if (setup_ip_handler_set) {
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, setup_ip_handler);
        setup_ip_handler_set = false;
    }
    if (setup_wifi_handler_set) {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, setup_wifi_handler);
        setup_wifi_handler_set = false;
    }
    esp_wifi_deinit();
fail:
    if (setup_sta) { esp_netif_destroy_default_wifi(setup_sta); setup_sta = NULL; }
    if (setup_ap) { esp_netif_destroy_default_wifi(setup_ap); setup_ap = NULL; }
    ESP_LOGE(TAG, "JR_WIFI_SETUP_ERROR result=%s", esp_err_to_name(err));
    setup_memory("error");
    return err;
}

void jr_wifi_provisioning_button_init(void) {
    gpio_config_t config = {
        .pin_bit_mask = 1ULL << GPIO_NUM_0,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&config);
    button_ready = err == ESP_OK;
    if (!button_ready) ESP_LOGW(TAG, "JR_WIFI_SETUP_BUTTON unavailable=%s", esp_err_to_name(err));
}

void jr_wifi_provisioning_button_poll(uint32_t now_ms) {
    if (!button_ready || jr_mode_get() != JR_MODE_IDLE) {
        button_down_ms = 0;
        button_armed = false;
        return;
    }
    if (gpio_get_level(GPIO_NUM_0) == 0) {
        if (!button_down_ms) button_down_ms = now_ms ? now_ms : 1;
        if (!button_armed && (uint32_t)(now_ms - button_down_ms) >= JR_SETUP_BUTTON_MS) {
            button_armed = true;
            ESP_LOGI(TAG, "JR_WIFI_SETUP_BUTTON held_ms=%u; release BOOT to enter setup",
                     JR_SETUP_BUTTON_MS);
        }
        return;
    }
    button_down_ms = 0;
    if (!button_armed) return;
    button_armed = false;
    /* GPIO0 must be high when the chip reboots, or it enters the ROM flasher. */
    setup_request = JR_SETUP_MAGIC;
    setup_request_inverse = ~JR_SETUP_MAGIC;
    esp_restart();
}
