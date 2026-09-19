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
#include "jr_portal_web_v1.h"
#include "jr_portal_v16.h"

#if __has_include("jr_https_material_local.h")
#include "jr_https_material_local.h"
#else
#define JR_HTTPS_LOCAL_MATERIAL_AVAILABLE 0
#endif

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
    static const char inject[]="<script src='/v16.js'></script>";
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

static esp_err_t web_status_handler(httpd_req_t *req) {
    char response[JR_RESPONSE_MAX_BYTES];
    bool ok=jr_format_status(response,sizeof(response));
    return reply(req,ok?"200 OK":"500 Internal Server Error",ok?response:"JR_ERROR status_buffer");
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

static esp_err_t register_routes(httpd_handle_t server) {
    const httpd_uri_t routes[]={
        {.uri="/",.method=HTTP_GET,.handler=web_root_handler},
        {.uri="/v16.js",.method=HTTP_GET,.handler=web_v16_js_handler},
        {.uri="/status",.method=HTTP_GET,.handler=web_status_handler},
        {.uri="/cmd",.method=HTTP_POST,.handler=web_cmd_handler},
        {.uri="/cmd",.method=HTTP_GET,.handler=legacy_get_handler},
        {.uri="/capture",.method=HTTP_GET,.handler=web_capture_handler},
        {.uri="/mic-record",.method=HTTP_GET,.handler=jr_mic_record_wav_handler},
        {.uri="/mic-live",.method=HTTP_GET,.handler=jr_mic_live_pcm_handler},
        {.uri="/live-diag",.method=HTTP_GET,.handler=jr_mic_live_diag_handler},
        {.uri="/audio",.method=HTTP_POST,.handler=jr_audio_receive_wav_handler},
        {.uri="/autofocus",.method=HTTP_GET,.handler=disabled_camera_handler},
        {.uri="/manual-focus",.method=HTTP_GET,.handler=disabled_camera_handler}
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
    config.max_uri_handlers=12;
    config.lru_purge_enable=true;
    esp_err_t err=httpd_start(&web_server,&config);
    if (err!=ESP_OK) { web_server=NULL; ESP_LOGE("jrbot_portal","httpd_start=%s",esp_err_to_name(err)); return; }
    err=register_routes(web_server);
    if (err!=ESP_OK) { httpd_stop(web_server); web_server=NULL; ESP_LOGE("jrbot_portal","http_register=%s",esp_err_to_name(err)); return; }
    ESP_LOGI("jrbot_portal","Portal HTTP iniciado port=80 V1.6");
}

static void start_https(void) {
#if JR_HTTPS_LOCAL_MATERIAL_AVAILABLE
    httpd_ssl_config_t config=HTTPD_SSL_CONFIG_DEFAULT();
    config.port_secure=443;
    config.httpd.ctrl_port=32769;
    config.httpd.stack_size=16384;
    config.httpd.max_uri_handlers=12;
    config.httpd.lru_purge_enable=true;
    config.servercert=(const uint8_t *)JR_HTTPS_CERT_PEM;
    config.servercert_len=sizeof(JR_HTTPS_CERT_PEM);
    config.prvtkey_pem=(const uint8_t *)JR_HTTPS_KEY_PEM;
    config.prvtkey_len=sizeof(JR_HTTPS_KEY_PEM);
    esp_err_t err=httpd_ssl_start(&https_server,&config);
    if (err!=ESP_OK) { https_server=NULL; ESP_LOGE("jrbot_portal","https_start=%s",esp_err_to_name(err)); return; }
    err=register_routes(https_server);
    if (err!=ESP_OK) { httpd_ssl_stop(https_server); https_server=NULL; ESP_LOGE("jrbot_portal","https_register=%s",esp_err_to_name(err)); return; }
    ESP_LOGI("jrbot_portal","JR_HTTPS_READY port=443 ctrl_port=32769 cert=local V1.6");
#else
    ESP_LOGW("jrbot_portal","JR_HTTPS_DISABLED material_local_ausente execute_CONFIGURAR_HTTPS_V1");
#endif
}

void jr_portal_start(void) {
    if ((web_server || https_server) || !jr_wifi_network_ready()) return;
    start_http();
    start_https();
    if (web_server || https_server) {
        ESP_LOGI("jrbot_portal","Portal V1.6.3.2 iniciado http=%d https=%d camera=/capture mic_live=/mic-live diag=/live-diag audio=/audio",
                 web_server?1:0,https_server?1:0);
    }
}
