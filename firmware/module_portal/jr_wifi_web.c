#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "jr_mode_manager.h"
#include "jr_wifi.h"
#include "jr_wifi_web.h"

/* This control interface is registered ONLY on the existing HTTPS server.
 * The HTTP /cmd interface continues to block all wifi_* commands. */
#define JR_WIFI_WEB_RESPONSE_BYTES 1800
#define JR_WIFI_WEB_BODY_BYTES 480

static const char *TAG = "jrbot_wifi_web";
static esp_timer_handle_t restart_timer;

static esp_err_t json_reply(httpd_req_t *req, const char *status, const char *body)
{
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
    return httpd_resp_sendstr(req, body);
}

static esp_err_t json_error(httpd_req_t *req, const char *status, const char *error)
{
    /* All error literals are server-owned, not user-controlled. */
    char message[176];
    snprintf(message, sizeof(message), "{\"ok\":false,\"error\":\"%s\"}", error);
    return json_reply(req, status, message);
}

static void encode_ssid(const uint8_t *ssid, char encoded[97])
{
    static const char hex[] = "0123456789ABCDEF";
    size_t used = 0;
    for (unsigned i = 0; i < 32 && ssid[i]; i++) {
        unsigned char c = ssid[i];
        encoded[used++] = '%';
        encoded[used++] = hex[c >> 4];
        encoded[used++] = hex[c & 0x0f];
    }
    encoded[used] = 0;
}

/* Return status with no scanning and no new persistent allocations. */
static esp_err_t wifi_status(httpd_req_t *req)
{
    jr_wifi_status_t status;
    jr_wifi_get_status(&status);
    wifi_ap_record_t ap = {0};
    bool associated = status.connected && esp_wifi_sta_get_ap_info(&ap) == ESP_OK;
    char ssid[97];
    if (associated)
        encode_ssid(ap.ssid, ssid);
    else
        encode_ssid((const uint8_t *)jr_wifi_ssid(), ssid);

    char json[400];
    snprintf(json, sizeof(json),
             "{\"ok\":true,\"connected\":%s,\"network_ready\":%s,"
             "\"pending_restart\":%s,\"ssid_pct\":\"%s\",\"ip\":\"%s\","
             "\"rssi\":%d,\"rssi_valid\":%s,\"channel\":%u,\"authmode\":%u,"
             "\"mode\":\"%s\"}",
             status.connected ? "true" : "false",
             status.network_ready ? "true" : "false",
             status.pending_restart ? "true" : "false",
             ssid, status.ip,
             associated ? (int)ap.rssi : 0, associated ? "true" : "false",
             associated ? (unsigned)ap.primary : 0U,
             associated ? (unsigned)ap.authmode : 0U,
             jr_mode_name(jr_mode_get()));
    return json_reply(req, "200 OK", json);
}

static esp_err_t wifi_list(httpd_req_t *req, bool scan)
{
    if (scan && jr_mode_get() != JR_MODE_IDLE)
        return json_error(req, "409 Conflict", "Pare a Live e o modo autonomo para buscar redes");

    /* One temporary PSRAM buffer, released immediately after the response. */
    char *buf = heap_caps_malloc(JR_WIFI_WEB_RESPONSE_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf) return json_error(req, "503 Service Unavailable", "PSRAM indisponivel");
    bool ok = scan ? jr_wifi_scan(buf, JR_WIFI_WEB_RESPONSE_BYTES)
                   : jr_wifi_profiles_list(buf, JR_WIFI_WEB_RESPONSE_BYTES);
    const char *prefix = scan ? "JR_WIFI_SCAN " : "JR_WIFI_PROFILES ";
    size_t prefix_len = strlen(prefix);
    if (!ok || strncmp(buf, prefix, prefix_len)) {
        ESP_LOGW(TAG, "Wi-Fi %s failed", scan ? "scan" : "profiles");
        heap_caps_free(buf);
        return json_error(req, "409 Conflict", "Consulta Wi-Fi indisponivel");
    }
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
    esp_err_t err = httpd_resp_sendstr(req, buf + prefix_len);
    heap_caps_free(buf);
    return err;
}

static esp_err_t wifi_get(httpd_req_t *req)
{
    char query[56], view[20] = "status";
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "view", view, sizeof(view)) != ESP_OK)
            return json_error(req, "400 Bad Request", "Parametro view invalido");
    }
    if (!strcmp(view, "status")) return wifi_status(req);
    if (!strcmp(view, "profiles")) return wifi_list(req, false);
    if (!strcmp(view, "scan")) return wifi_list(req, true);
    return json_error(req, "400 Bad Request", "Consulta desconhecida");
}

/* Browser requests carry a custom header and JSON content type, requiring
 * a cross-origin preflight that this server never authorizes. */
static bool allow_write(httpd_req_t *req)
{
    char marker[4], type[64], origin[120], host[100];
    if (httpd_req_get_hdr_value_str(req, "X-JrBot-WiFi", marker, sizeof(marker)) != ESP_OK ||
        strcmp(marker, "1")) return false;
    if (httpd_req_get_hdr_value_str(req, "Content-Type", type, sizeof(type)) != ESP_OK ||
        strncmp(type, "application/json", 16)) return false;
    if (httpd_req_get_hdr_value_str(req, "Origin", origin, sizeof(origin)) == ESP_OK) {
        if (httpd_req_get_hdr_value_str(req, "Host", host, sizeof(host)) != ESP_OK) return false;
        char expected[112];
        if (snprintf(expected, sizeof(expected), "https://%s", host) >= sizeof(expected))
            return false;
        if (strcmp(origin, expected)) return false;
    }
    return true;
}

static void restart_callback(void *arg)
{
    (void)arg;
    esp_restart();
}

static esp_err_t schedule_restart(void)
{
    if (!restart_timer) {
        const esp_timer_create_args_t config = {
            .callback = restart_callback, .name = "wifi_switch"
        };
        esp_err_t err = esp_timer_create(&config, &restart_timer);
        if (err != ESP_OK) return err;
    }
    if (esp_timer_is_active(restart_timer)) return ESP_ERR_INVALID_STATE;
    return esp_timer_start_once(restart_timer, 2500000);
}

static bool json_string(const cJSON *root, const char *name, const char **out)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
    if (!cJSON_IsString(item) || !item->valuestring) return false;
    *out = item->valuestring;
    return true;
}

static const char *optional_string(const cJSON *root, const char *name)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
    return cJSON_IsString(item) && item->valuestring ? item->valuestring : "";
}

static esp_err_t wifi_post(httpd_req_t *req)
{
    if (!allow_write(req))
        return json_error(req, "403 Forbidden", "Use o painel HTTPS do JrBot");
    if (req->content_len <= 0 || req->content_len >= JR_WIFI_WEB_BODY_BYTES)
        return json_error(req, "413 Payload Too Large", "Requisicao invalida");
    if (jr_mode_get() != JR_MODE_IDLE)
        return json_error(req, "409 Conflict", "Pare a Live e o modo autonomo antes de alterar a rede");

    char body[JR_WIFI_WEB_BODY_BYTES] = {0};
    int received = 0;
    while (received < req->content_len) {
        int n = httpd_req_recv(req, body + received, req->content_len - received);
        if (n <= 0) {
            memset(body, 0, sizeof(body));
            return json_error(req, "400 Bad Request", "Requisicao incompleta");
        }
        received += n;
    }
    cJSON *json = cJSON_ParseWithLength(body, (size_t)received);
    const char *action = "";
    if (!json || !json_string(json, "action", &action)) {
        cJSON_Delete(json);
        memset(body, 0, sizeof(body));
        return json_error(req, "400 Bad Request", "Acao invalida");
    }

    bool success = false;
    bool schedule = false;
    char result[180] = {0};
    if (!strcmp(action, "save")) {
        const char *ssid = "", *password = "";
        const cJSON *static_item = cJSON_GetObjectItemCaseSensitive(json, "use_static");
        if (json_string(json, "ssid", &ssid) && json_string(json, "password", &password) &&
            strlen(ssid) >= 1 && strlen(ssid) <= 32 && strlen(password) <= 64 &&
            (!static_item || cJSON_IsBool(static_item))) {
            success = jr_wifi_profile_save(
                ssid, password, "jrbot", cJSON_IsTrue(static_item),
                optional_string(json, "ip"), optional_string(json, "gateway"),
                optional_string(json, "mask"), optional_string(json, "dns1"),
                optional_string(json, "dns2"), result, sizeof(result));
        }
        /* The current station remains connected. The saved profile is not selected yet. */
    } else if (!strcmp(action, "use") || !strcmp(action, "delete")) {
        const cJSON *item = cJSON_GetObjectItemCaseSensitive(json, "id");
        if (cJSON_IsNumber(item) && item->valuedouble == (double)item->valueint &&
            item->valueint >= 0 && item->valueint <= 3) {
            if (!strcmp(action, "delete"))
                success = jr_wifi_profile_delete((unsigned)item->valueint, result, sizeof(result));
            else
                success = jr_wifi_profile_use((unsigned)item->valueint, result, sizeof(result));
        }
    } else if (!strcmp(action, "restart")) {
        jr_wifi_status_t status;
        jr_wifi_get_status(&status);
        if (status.pending_restart) {
            success = schedule_restart() == ESP_OK;
            schedule = success;
        }
    }
    /* Do not keep password or SSID secrets in temporary request allocations. */
    cJSON *pass = cJSON_GetObjectItemCaseSensitive(json, "password");
    if (cJSON_IsString(pass) && pass->valuestring)
        memset(pass->valuestring, 0, strlen(pass->valuestring));
    cJSON_Delete(json);
    memset(body, 0, sizeof(body));

    if (!success) return json_error(req, "400 Bad Request",
                                    "Nao foi possivel aplicar: confira os dados ou selecione outra rede");
    if (schedule) {
        ESP_LOGI(TAG, "Reiniciando para aplicar a rede selecionada");
        return json_reply(req, "200 OK", "{\"ok\":true,\"restarting\":true}");
    }
    jr_wifi_status_t status;
    jr_wifi_get_status(&status);
    if (!strcmp(action, "delete")) {
        /* A deletion never disconnects the running station. Deleting the
         * selected profile opens provisioning on the next reboot. */
        return json_reply(req, "200 OK", status.pending_restart ?
            "{\"ok\":true,\"deleted\":true,\"pending_restart\":true}" :
            "{\"ok\":true,\"deleted\":true,\"pending_restart\":false}");
    }
    return json_reply(req, "200 OK", status.pending_restart ?
        "{\"ok\":true,\"pending_restart\":true}" :
        "{\"ok\":true,\"pending_restart\":false}");
}

esp_err_t jr_wifi_web_register_routes(httpd_handle_t server)
{
    if (!server) return ESP_ERR_INVALID_ARG;
    const httpd_uri_t routes[] = {
        {.uri = "/wifi-manager", .method = HTTP_GET, .handler = wifi_get},
        {.uri = "/wifi-manager", .method = HTTP_POST, .handler = wifi_post},
    };
    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
        esp_err_t err = httpd_register_uri_handler(server, &routes[i]);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}
