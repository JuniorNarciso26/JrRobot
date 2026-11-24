#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_http_server.h"
#include "esp_https_server.h"
#include "esp_log.h"
#include "jr_board.h"
#include "jr_commands.h"
#include "jr_camera_diag.h"
#include "jr_mic.h"
#include "jr_wifi.h"
#include "jr_portal.h"
#include "jr_audio_receive.h"
#include "jr_webrtc_audio.h"
#include "jr_tts.h"
#include "jr_resource_manager.h"
#include "cJSON.h"
#include "jr_wifi_web.h"
#include "jr_memory.h"
#include "jr_memory_tasks.h"
#include "jr_memory_full.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "jr_portal_web_v1.h"
#include "jr_portal_v16.h"

/*
 * The experimental develop/vTTS firmware must never be released with HTTP only.
 * The installer prepares this gitignored header BEFORE compiling. A missing or
 * disabled header is now a build failure, not a silent loss of HTTPS/WebRTC.
 * Existing production and non-vTTS branch behavior remains unchanged.
 */
#if defined(JRBOT_WITH_VTTS) && JRBOT_WITH_VTTS
#include "jr_https_material_local.h"
#if !defined(JR_HTTPS_LOCAL_MATERIAL_AVAILABLE) || JR_HTTPS_LOCAL_MATERIAL_AVAILABLE != 1
#error "Develop vTTS HTTPS material is not enabled; run INSTALAR.bat again."
#endif
#elif __has_include("jr_https_material_local.h")
#include "jr_https_material_local.h"
#else
#define JR_HTTPS_LOCAL_MATERIAL_AVAILABLE 0
#endif

extern const char jrskill_app_js_start[] asm("_binary_jrskill_app_js_start");
extern const char jrskill_app_js_end[] asm("_binary_jrskill_app_js_end");
extern const char memory_panel_js_start[] asm("_binary_memory_panel_js_start");
extern const char wifi_panel_js_start[] asm("_binary_wifi_panel_js_start");
extern const char voice_panel_js_start[] asm("_binary_voice_panel_js_start");
extern const char memory_build_json_start[] asm("_binary_jr_memory_build_json_start");

static httpd_handle_t web_server;
static httpd_handle_t https_server;
static uint32_t live_frame_count;

static esp_err_t reply(httpd_req_t *req,const char *status,const char *text) {
    httpd_resp_set_status(req,status);
    httpd_resp_set_type(req,"text/plain; charset=utf-8");
    httpd_resp_set_hdr(req,"Cache-Control","no-store");
    return httpd_resp_send(req,text,HTTPD_RESP_USE_STRLEN);
}

static esp_err_t web_root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req,"text/html; charset=utf-8");
    httpd_resp_set_hdr(req,"Cache-Control","no-store");
    const char *marker=strstr(JRBOT_WEB_V1_HTML,"</body>");
    if (!marker) return httpd_resp_send(req,JRBOT_WEB_V1_HTML,HTTPD_RESP_USE_STRLEN);
    static const char inject[]="<script src='/v16.js'></script><script src='/jrskill-app.js'></script><script src='/memory-panel.js'></script><script src='/wifi-panel.js'></script><script src='/voice-panel.js'></script>";
    esp_err_t err=httpd_resp_send_chunk(req,JRBOT_WEB_V1_HTML,(ssize_t)(marker-JRBOT_WEB_V1_HTML));
    if (err==ESP_OK) err=httpd_resp_send_chunk(req,inject,HTTPD_RESP_USE_STRLEN);
    if (err==ESP_OK) err=httpd_resp_send_chunk(req,marker,HTTPD_RESP_USE_STRLEN);
    (void)httpd_resp_send_chunk(req,NULL,0);
    return err;
}

static esp_err_t web_v16_js_handler(httpd_req_t *req) {
    httpd_resp_set_type(req,"application/javascript; charset=utf-8");
    httpd_resp_set_hdr(req,"Cache-Control","no-store");
    return httpd_resp_send(req,JRBOT_V16_JS,HTTPD_RESP_USE_STRLEN);
}

static esp_err_t web_jrskill_app_js_handler(httpd_req_t *req) {
    httpd_resp_set_type(req,"application/javascript; charset=utf-8");
    httpd_resp_set_hdr(req,"Cache-Control","no-store");
    return httpd_resp_send(req,jrskill_app_js_start,HTTPD_RESP_USE_STRLEN);
}

static esp_err_t web_status_handler(httpd_req_t *req) {
    char response[JR_RESPONSE_MAX_BYTES];
    bool ok=jr_format_status(response,sizeof(response));
    return reply(req,ok?"200 OK":"500 Internal Server Error",ok?response:"JR_ERROR status_buffer");
}

static esp_err_t web_memory_handler(httpd_req_t *req) {
    char query[24], value[4];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK &&
        httpd_query_key_value(query, "build", value, sizeof(value)) == ESP_OK &&
        !strcmp(value, "1")) {
        httpd_resp_set_type(req, "application/json; charset=utf-8");
        httpd_resp_set_hdr(req, "Cache-Control", "no-store");
        return httpd_resp_send(req, memory_build_json_start, HTTPD_RESP_USE_STRLEN);
    }
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK &&
        httpd_query_key_value(query, "full", value, sizeof(value)) == ESP_OK &&
        !strcmp(value, "1")) {
        char *report = NULL;
        size_t length = 0;
        esp_err_t result = jr_memory_full_json(&report, &length);
        if (result != ESP_OK) return reply(req, "503 Service Unavailable", esp_err_to_name(result));
        httpd_resp_set_type(req, "application/json; charset=utf-8");
        httpd_resp_set_hdr(req, "Cache-Control", "no-store");
        result = httpd_resp_send(req, report, (ssize_t)length);
        heap_caps_free(report);
        return result;
    }
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK &&
        httpd_query_key_value(query, "tasks", value, sizeof(value)) == ESP_OK &&
        !strcmp(value, "1")) {
        char tasks[JR_RESPONSE_MAX_BYTES];
        bool ok = jr_memory_tasks_format(tasks, sizeof(tasks));
        return reply(req, ok ? "200 OK" : "503 Service Unavailable", tasks);
    }
    char response[640];
    bool ok = jr_memory_format(response, sizeof(response));
    return reply(req, ok ? "200 OK" : "500 Internal Server Error",
                 ok ? response : "JR_ERROR memory_buffer");
}

static esp_err_t web_wifi_js_handler(httpd_req_t *req) {
    httpd_resp_set_type(req,"application/javascript; charset=utf-8");
    httpd_resp_set_hdr(req,"Cache-Control","no-store");
    return httpd_resp_send(req,wifi_panel_js_start,HTTPD_RESP_USE_STRLEN);
}

static esp_err_t web_memory_js_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/javascript; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, memory_panel_js_start, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t web_voice_js_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/javascript; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, voice_panel_js_start, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t web_voice_get_handler(httpd_req_t *req) {
    jr_tts_status_t st = {0};
    jr_tts_get_status(&st);
    jr_i2s_resource_status_t audio = {0};
    jr_resource_i2s_get_status(&audio);
    char json[600];
    snprintf(json, sizeof(json),
             "{\"ok\":true,\"available\":%s,\"busy\":%s,\"enabled\":%s,"
             "\"model\":\"%c\",\"volume\":%d,\"gain_db\":%d,\"debuzz_db\":%d,"
             "\"speed\":%.2f,\"expression\":%.2f,\"utterances\":%lu,"
             "\"last_audio_ms\":%lu,\"last_synth_ms\":%lu,\"last_rtf\":%.3f,"
             "\"last_error\":\"%s\",\"psram_free\":%u,\"i2s_owner\":\"%s\"}",
             st.available ? "true" : "false", st.busy ? "true" : "false",
             st.settings.enabled ? "true" : "false", st.settings.model,
             st.settings.volume, st.settings.gain_db, st.settings.debuzz_db,
             (double)st.settings.speed, (double)st.settings.expression,
             (unsigned long)st.utterances, (unsigned long)st.last_audio_ms,
             (unsigned long)st.last_synth_ms, (double)st.last_rtf,
             esp_err_to_name(st.last_error),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             jr_i2s_owner_name(audio.owner));
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t voice_error(httpd_req_t *req, const char *code, const char *status) {
    char json[160];
    snprintf(json, sizeof(json), "{\"ok\":false,\"error\":\"%s\"}", code);
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t web_voice_post_handler(httpd_req_t *req) {
    /* Speech controls must not be exposed over plaintext HTTP on a LAN. */
    if (!https_server || req->handle != https_server)
        return voice_error(req, "use_https", "403 Forbidden");
    char header[4];
    if (httpd_req_get_hdr_value_str(req, "X-JrBot-Voice", header, sizeof(header)) != ESP_OK || strcmp(header, "1"))
        return voice_error(req, "required_header", "403 Forbidden");
    if (req->content_len == 0 || req->content_len > 512)
        return voice_error(req, "payload_too_large", "413 Payload Too Large");

    char body[513];
    size_t read = 0;
    while (read < req->content_len) {
        int n = httpd_req_recv(req, body + read, req->content_len - read);
        if (n <= 0) return voice_error(req, "incomplete_request", "408 Request Timeout");
        read += (size_t)n;
    }
    if (memchr(body, '\0', read)) return voice_error(req, "invalid_input", "400 Bad Request");
    body[read] = '\0';
    cJSON *root = cJSON_Parse(body);
    if (!root) return voice_error(req, "invalid_json", "400 Bad Request");
    const cJSON *action = cJSON_GetObjectItemCaseSensitive(root, "action");
    esp_err_t err = ESP_ERR_INVALID_ARG;
    bool queued = false;
    if (cJSON_IsString(action) && !strcmp(action->valuestring, "configure")) {
        const cJSON *enabled = cJSON_GetObjectItemCaseSensitive(root, "enabled");
        const cJSON *model = cJSON_GetObjectItemCaseSensitive(root, "model");
        const cJSON *volume = cJSON_GetObjectItemCaseSensitive(root, "volume");
        const cJSON *gain = cJSON_GetObjectItemCaseSensitive(root, "gain_db");
        const cJSON *debuzz = cJSON_GetObjectItemCaseSensitive(root, "debuzz_db");
        const cJSON *speed = cJSON_GetObjectItemCaseSensitive(root, "speed");
        const cJSON *expression = cJSON_GetObjectItemCaseSensitive(root, "expression");
        if (cJSON_IsBool(enabled) && cJSON_IsString(model) && strlen(model->valuestring) == 1 &&
            cJSON_IsNumber(volume) && cJSON_IsNumber(gain) && cJSON_IsNumber(debuzz) &&
            cJSON_IsNumber(speed) && cJSON_IsNumber(expression)) {
            jr_tts_settings_t settings = {
                .enabled = cJSON_IsTrue(enabled),
                .model = model->valuestring[0],
                .volume = volume->valueint,
                .gain_db = gain->valueint,
                .debuzz_db = debuzz->valueint,
                .speed = (float)speed->valuedouble,
                .expression = (float)expression->valuedouble,
            };
            err = jr_tts_configure(&settings);
        }
    } else if (cJSON_IsString(action) && !strcmp(action->valuestring, "speak")) {
        const cJSON *text = cJSON_GetObjectItemCaseSensitive(root, "text");
        if (cJSON_IsString(text) && text->valuestring) {
            err = jr_tts_say(text->valuestring);
            queued = err == ESP_OK;
        }
    }
    cJSON_Delete(root);
    if (err != ESP_OK) {
        return voice_error(req, err == ESP_ERR_INVALID_STATE ? "voice_busy_or_autonomous" :
                                err == ESP_ERR_NOT_SUPPORTED ? "vtts_not_compiled" :
                                err == ESP_ERR_INVALID_ARG ? "invalid_voice_settings" :
                                esp_err_to_name(err),
                           err == ESP_ERR_NOT_SUPPORTED ? "503 Service Unavailable" : "400 Bad Request");
    }
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, queued ? "{\"ok\":true,\"queued\":true}" :
                                        "{\"ok\":true,\"saved\":true}", HTTPD_RESP_USE_STRLEN);
}

static void camera_request_options(httpd_req_t *req,jr_camera_size_t *size,int *quality,bool *live) {
    if (size) *size=JR_CAMERA_SIZE_VGA;
    if (quality) *quality=12;
    if (live) *live=false;
    char query[160]={0};
    if (httpd_req_get_url_query_str(req,query,sizeof(query))!=ESP_OK) return;
    char value[32]={0};
    if (size && httpd_query_key_value(query,"size",value,sizeof(value))==ESP_OK) {
        jr_camera_size_t parsed;
        if (jr_camera_parse_size(value,&parsed)) *size=parsed;
    }
    memset(value,0,sizeof(value));
    if (quality && httpd_query_key_value(query,"quality",value,sizeof(value))==ESP_OK) {
        int q=atoi(value);
        if (q<4) q=4;
        if (q>30) q=30;
        *quality=q;
    }
    memset(value,0,sizeof(value));
    if (live && httpd_query_key_value(query,"live",value,sizeof(value))==ESP_OK) {
        *live=!strcmp(value,"1") || !strcmp(value,"true");
    }
}

static esp_err_t web_capture_handler(httpd_req_t *req) {
    jr_camera_size_t size=JR_CAMERA_SIZE_VGA;
    int quality=12;
    bool live=false;
    camera_request_options(req,&size,&quality,&live);

    jr_camera_jpeg_t frame={0};
    esp_err_t err=jr_camera_capture_jpeg_ex(&frame,size,quality);
    if (err!=ESP_OK) {
        char message[128];
        snprintf(message,sizeof(message),"JR_CAMERA_ERROR capture=%s size=%s quality=%d",
                 esp_err_to_name(err),jr_camera_size_name(size),quality);
        return reply(req,"503 Service Unavailable",message);
    }

    char width[16],height[16],pid[16],bytes[24],quality_text[16];
    snprintf(width,sizeof(width),"%u",frame.width);
    snprintf(height,sizeof(height),"%u",frame.height);
    snprintf(pid,sizeof(pid),"0x%04X",frame.pid);
    snprintf(bytes,sizeof(bytes),"%u",(unsigned)frame.len);
    snprintf(quality_text,sizeof(quality_text),"%d",frame.jpeg_quality);
    httpd_resp_set_type(req,"image/jpeg");
    httpd_resp_set_hdr(req,"Cache-Control","no-store, no-cache, must-revalidate");
    httpd_resp_set_hdr(req,"X-JrBot-Camera","OV5640");
    httpd_resp_set_hdr(req,"X-JrBot-Width",width);
    httpd_resp_set_hdr(req,"X-JrBot-Height",height);
    httpd_resp_set_hdr(req,"X-JrBot-PID",pid);
    httpd_resp_set_hdr(req,"X-JrBot-Bytes",bytes);
    httpd_resp_set_hdr(req,"X-JrBot-Size",jr_camera_size_name(frame.size));
    httpd_resp_set_hdr(req,"X-JrBot-JPEG-Quality",quality_text);
    err=httpd_resp_send(req,(const char *)frame.data,(ssize_t)frame.len);
    if (err==ESP_OK) {
        if (live) {
            live_frame_count++;
            if ((live_frame_count%25u)==0u) {
                ESP_LOGI("jrbot_portal","JR_CAMERA_LIVE frames=%lu size=%ux%u quality=%d bytes=%u",
                         (unsigned long)live_frame_count,frame.width,frame.height,frame.jpeg_quality,(unsigned)frame.len);
            }
        } else {
            ESP_LOGI("jrbot_portal","JR_CAMERA_PHOTO size=%ux%u profile=%s quality=%d bytes=%u pid=0x%04X",
                     frame.width,frame.height,jr_camera_size_name(frame.size),frame.jpeg_quality,(unsigned)frame.len,frame.pid);
        }
    } else {
        ESP_LOGW("jrbot_portal","Falha enviando JPEG OV5640: %s",esp_err_to_name(err));
    }
    jr_camera_jpeg_release(&frame);
    return err;
}

static bool network_command_allowed(const char *cmd) {
    while (*cmd==' ') cmd++;
    char verb[32]; size_t n=0;
    while (cmd[n] && cmd[n]!=' ' && n<sizeof(verb)-1) {
        verb[n]=(cmd[n]>='A'&&cmd[n]<='Z')?cmd[n]+32:cmd[n]; n++;
    }
    verb[n]=0;
    if (!strcmp(verb,"camera_test")) return false;
    if (strlen(cmd)>=5) {
        char prefix[6];
        for(int i=0;i<5;i++) prefix[i]=(cmd[i]>='A'&&cmd[i]<='Z')?cmd[i]+32:cmd[i];
        prefix[5]=0; if (!strcmp(prefix,"wifi_")) return false;
    }
    return true;
}

static esp_err_t web_cmd_handler(httpd_req_t *req) {
    char flag[4];
    if (httpd_req_get_hdr_value_str(req,"X-JrBot-Command",flag,sizeof(flag))!=ESP_OK || strcmp(flag,"1"))
        return reply(req,"403 Forbidden","JR_ERROR cabecalho_obrigatorio");
    if (req->content_len==0 || req->content_len>JR_COMMAND_MAX_BYTES) {
        reply(req,"413 Payload Too Large","JR_ERROR tamanho_comando");
        return ESP_FAIL;
    }
    char cmd[JR_COMMAND_MAX_BYTES+1]; size_t read=0;
    while (read<req->content_len) {
        int n=httpd_req_recv(req,cmd+read,req->content_len-read);
        if (n<=0) { reply(req,"408 Request Timeout","JR_ERROR corpo_incompleto"); return ESP_FAIL; }
        read+=(size_t)n;
    }
    if (memchr(cmd,0,read)) return reply(req,"400 Bad Request","JR_ERROR NUL_no_comando");
    cmd[read]='\0';
    if (!network_command_allowed(cmd)) return reply(req,"403 Forbidden","JR_ERROR comando_somente_serial");
    char response[JR_RESPONSE_MAX_BYTES];
    bool ok=jr_handle_command(cmd,response,sizeof(response));
    return reply(req,ok?"200 OK":"400 Bad Request",response);
}

static esp_err_t legacy_get_handler(httpd_req_t *req) {
    char query[128],raw[96],cmd[64];
    if (httpd_req_get_url_query_str(req,query,sizeof(query))!=ESP_OK ||
        httpd_query_key_value(query,"c",raw,sizeof(raw))!=ESP_OK ||
        !jr_decode_component(raw,strlen(raw),cmd,sizeof(cmd),true)) return reply(req,"400 Bad Request","JR_ERROR query_invalida");
    if (strcmp(cmd,"status") && strcmp(cmd,"help")) return reply(req,"405 Method Not Allowed","JR_ERROR use_POST_cmd");
    char response[JR_RESPONSE_MAX_BYTES]; bool ok=jr_handle_command(cmd,response,sizeof(response));
    return reply(req,ok?"200 OK":"400 Bad Request",response);
}

static esp_err_t disabled_camera_handler(httpd_req_t *req) {
    return reply(req,"503 Service Unavailable","JR_CAMERA_DISABLED recurso_indisponivel");
}

/* Log URI failures at the application boundary. TLS handshakes fail before
 * dispatch and remain handled by ESP-IDF; JR_NET_HEALTH samples the heap.
 * Strip the query so no URL parameters or credentials are emitted.
 */
#define JR_MONITOR_HTTP_HANDLER(fn) \
    static esp_err_t fn##_monitored(httpd_req_t *req) { \
        esp_err_t result=fn(req); \
        if(result!=ESP_OK) { \
            const char *uri=req->uri?req->uri:"-"; \
            size_t path_len=strcspn(uri,"?"); \
            if(path_len>96U)path_len=96U; \
            ESP_LOGW("jrbot_portal","JR_HTTP_FAIL uptime_ms=%llu method=%s uri=%.*s result=%s(%d) internal_free=%u internal_largest=%u psram_free=%u", \
                (unsigned long long)(esp_timer_get_time()/1000ULL), \
                req->method==HTTP_POST?"POST":"GET",(int)path_len,uri, \
                esp_err_to_name(result),(int)result, \
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT), \
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT), \
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM)); \
        } \
        return result; \
    }

JR_MONITOR_HTTP_HANDLER(web_root_handler)
JR_MONITOR_HTTP_HANDLER(web_v16_js_handler)
JR_MONITOR_HTTP_HANDLER(web_jrskill_app_js_handler)
JR_MONITOR_HTTP_HANDLER(web_status_handler)
JR_MONITOR_HTTP_HANDLER(web_memory_handler)
JR_MONITOR_HTTP_HANDLER(web_memory_js_handler)
JR_MONITOR_HTTP_HANDLER(web_wifi_js_handler)
JR_MONITOR_HTTP_HANDLER(web_voice_js_handler)
JR_MONITOR_HTTP_HANDLER(web_voice_get_handler)
JR_MONITOR_HTTP_HANDLER(web_voice_post_handler)
JR_MONITOR_HTTP_HANDLER(web_cmd_handler)
JR_MONITOR_HTTP_HANDLER(legacy_get_handler)
JR_MONITOR_HTTP_HANDLER(web_capture_handler)
JR_MONITOR_HTTP_HANDLER(jr_mic_record_wav_handler)
JR_MONITOR_HTTP_HANDLER(jr_mic_live_pcm_handler)
JR_MONITOR_HTTP_HANDLER(jr_mic_stream_handler)
JR_MONITOR_HTTP_HANDLER(jr_mic_ws_handler)
JR_MONITOR_HTTP_HANDLER(jr_mic_live_diag_handler)
JR_MONITOR_HTTP_HANDLER(jr_audio_receive_wav_handler)
JR_MONITOR_HTTP_HANDLER(disabled_camera_handler)

#undef JR_MONITOR_HTTP_HANDLER

static esp_err_t register_routes(httpd_handle_t server) {
    const httpd_uri_t routes[]={
        {.uri="/",.method=HTTP_GET,.handler=web_root_handler_monitored},
        {.uri="/v16.js",.method=HTTP_GET,.handler=web_v16_js_handler_monitored},
        {.uri="/jrskill-app.js",.method=HTTP_GET,.handler=web_jrskill_app_js_handler_monitored},
        {.uri="/status",.method=HTTP_GET,.handler=web_status_handler_monitored},
        {.uri="/memory",.method=HTTP_GET,.handler=web_memory_handler_monitored},
        {.uri="/memory-panel.js",.method=HTTP_GET,.handler=web_memory_js_handler_monitored},
        {.uri="/wifi-panel.js",.method=HTTP_GET,.handler=web_wifi_js_handler_monitored},
        {.uri="/voice-panel.js",.method=HTTP_GET,.handler=web_voice_js_handler_monitored},
        {.uri="/voice",.method=HTTP_GET,.handler=web_voice_get_handler_monitored},
        {.uri="/voice",.method=HTTP_POST,.handler=web_voice_post_handler_monitored},
        {.uri="/cmd",.method=HTTP_POST,.handler=web_cmd_handler_monitored},
        {.uri="/cmd",.method=HTTP_GET,.handler=legacy_get_handler_monitored},
        {.uri="/capture",.method=HTTP_GET,.handler=web_capture_handler_monitored},
        {.uri="/mic-record",.method=HTTP_GET,.handler=jr_mic_record_wav_handler_monitored},
        {.uri="/mic-live",.method=HTTP_GET,.handler=jr_mic_live_pcm_handler_monitored},
        {.uri="/mic-stream",.method=HTTP_GET,.handler=jr_mic_stream_handler_monitored},
        {.uri="/mic-ws",.method=HTTP_GET,.handler=jr_mic_ws_handler_monitored,.is_websocket=true},
        {.uri="/live-diag",.method=HTTP_GET,.handler=jr_mic_live_diag_handler_monitored},
        {.uri="/audio",.method=HTTP_POST,.handler=jr_audio_receive_wav_handler_monitored},
        {.uri="/autofocus",.method=HTTP_GET,.handler=disabled_camera_handler_monitored},
        {.uri="/manual-focus",.method=HTTP_GET,.handler=disabled_camera_handler_monitored}
    };
    for (size_t i=0;i<sizeof(routes)/sizeof(routes[0]);i++) {
        esp_err_t err=httpd_register_uri_handler(server,&routes[i]);
        if (err!=ESP_OK) return err;
    }
    return ESP_OK;
}

static void start_http(void) {
    httpd_config_t config=HTTPD_DEFAULT_CONFIG();
    config.server_port=80;
    config.stack_size=12288;
    config.max_uri_handlers=21;
    config.lru_purge_enable=true;
    esp_err_t err=httpd_start(&web_server,&config);
    if (err!=ESP_OK) { web_server=NULL; ESP_LOGE("jrbot_portal","httpd_start=%s",esp_err_to_name(err)); return; }
    err=register_routes(web_server);
    if (err!=ESP_OK) { httpd_stop(web_server); web_server=NULL; ESP_LOGE("jrbot_portal","http_register=%s",esp_err_to_name(err)); return; }
    ESP_LOGI("jrbot_portal","Portal HTTP iniciado port=80 V1.6");
}

static void start_https(void) {
#if JR_HTTPS_LOCAL_MATERIAL_AVAILABLE
    ESP_LOGI("jrbot_portal","JR_HTTPS_MATERIAL_COMPILED local=1 starting=443");
    httpd_ssl_config_t config=HTTPD_SSL_CONFIG_DEFAULT();
    config.port_secure=443;
    config.httpd.ctrl_port=32769;
    config.httpd.stack_size=16384;
    config.httpd.max_uri_handlers=29; /* 17 portal routes + 6 WebRTC/audio lab routes */
    config.httpd.max_open_sockets=7;
    config.httpd.lru_purge_enable=true;
    config.servercert=(const uint8_t *)JR_HTTPS_CERT_PEM;
    config.servercert_len=sizeof(JR_HTTPS_CERT_PEM);
    config.prvtkey_pem=(const uint8_t *)JR_HTTPS_KEY_PEM;
    config.prvtkey_len=sizeof(JR_HTTPS_KEY_PEM);
    esp_err_t err=httpd_ssl_start(&https_server,&config);
    if (err!=ESP_OK) { https_server=NULL; ESP_LOGE("jrbot_portal","https_start=%s",esp_err_to_name(err)); return; }
    err=register_routes(https_server);
    if (err!=ESP_OK) { httpd_ssl_stop(https_server); https_server=NULL; ESP_LOGE("jrbot_portal","https_register=%s",esp_err_to_name(err)); return; }
    err=jr_webrtc_audio_register_routes(https_server);
    if (err==ESP_OK) err=jr_wifi_web_register_routes(https_server);
    if (err!=ESP_OK) { httpd_ssl_stop(https_server); https_server=NULL; ESP_LOGE("jrbot_portal","webrtc_audio_register=%s",esp_err_to_name(err)); return; }
    ESP_LOGI("jrbot_portal","JR_HTTPS_READY port=443 ctrl_port=32769 cert=local V1.6 WebRTCAudio=/webrtc-audio");
#else
    ESP_LOGW("jrbot_portal","JR_HTTPS_DISABLED material_local_ausente execute_CONFIGURAR_HTTPS_V1");
#endif
}

void jr_portal_start(void) {
    if ((web_server || https_server) || !jr_wifi_network_ready()) return;
    start_http();
    start_https();
    if (web_server || https_server) {
        ESP_LOGI("jrbot_portal","Portal V1.6.3 iniciado http=%d https=%d camera=/capture WebRTCAudio=/webrtc-audio legacy_mic_ws=/mic-ws",
                 web_server?1:0,https_server?1:0);
    }
}

