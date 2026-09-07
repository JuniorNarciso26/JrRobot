#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "jr_board.h"
#include "jr_commands.h"
#include "jr_camera_diag.h"
#include "jr_wifi.h"
#include "jr_portal.h"

static httpd_handle_t web_server;
static const char WEB_HTML[]=
"<!doctype html><html lang='pt-br'><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>JrBot V2</title><style>body{max-width:860px;margin:24px auto;padding:16px;font:16px Arial;background:#101520;color:#eee}button,input{margin:5px;padding:12px}pre{white-space:pre-wrap;overflow-wrap:anywhere}section{padding:16px;border:1px solid #485064;margin:16px 0}img{max-width:100%;border:1px solid #485064}</style>"
"<h1>JrBot - robo com IA em desenvolvimento</h1><p>OLED: SDA GPIO1 / SCL GPIO2. Camera OV5640 disponivel sob demanda.</p>"
"<p>Somente rede local confiavel: este portal nao possui autenticacao/TLS. Nao exponha a Internet. Configure Wi-Fi pela Serial.</p>"
"<section><pre id='state'></pre><button onclick='refresh()'>Atualizar estado</button></section>"
"<section><button onclick='photo()'>Tirar foto</button><br><img id='photo' alt='Foto da OV5640'></section>"
"<section><div id='faces'></div></section><section><label>Volume <input id='vol' type='number' min='0' max='100' value='35'></label>"
"<button onclick='run(\"audio_volume \"+document.getElementById(\"vol\").value)'>Aplicar</button><button id='audio' disabled onclick='run(\"audio_test\")'>Testar som</button>"
"<p>GPIO21/41/42/47 ocupados pelo conjunto de audio; nao reutilizar.</p></section><pre id='log'></pre>"
"<script>const faces=['neutro','feliz','triste','animado','bravo','surpreso','pensando','cetico','sono','confuso','piscando','amor','brincalhao','preocupado','cool','bateria','demo'];"
"for(const c of faces){const b=document.createElement('button');b.textContent=c;b.onclick=()=>run(c);document.getElementById('faces').appendChild(b)}"
"function log(s){const el=document.getElementById('log');el.textContent=(s+'\\n'+el.textContent).slice(0,18000)}"
"function photo(){document.getElementById('photo').src='/capture?t='+Date.now()}"
"async function send(c){const r=await fetch('/cmd',{method:'POST',headers:{'Content-Type':'text/plain','X-JrBot-Command':'1'},body:c});const t=await r.text();if(!r.ok)throw new Error(t);return t}"
"async function run(c){try{log(await send(c));await refresh()}catch(e){log('ERRO: '+e.message)}}"
"async function refresh(){try{const r=await fetch('/status');const t=await r.text();if(!r.ok)throw new Error(t);document.getElementById('state').textContent=t;document.getElementById('audio').disabled=!(t.includes('audio=ready')||t.includes('audio=on_demand'))}catch(e){document.getElementById('state').textContent='Indisponivel: '+e.message}}refresh();setInterval(refresh,4000)</script></html>";

static esp_err_t reply(httpd_req_t *req,const char *status,const char *text) {
    httpd_resp_set_status(req,status);
    httpd_resp_set_type(req,"text/plain; charset=utf-8");
    httpd_resp_set_hdr(req,"Cache-Control","no-store");
    return httpd_resp_send(req,text,HTTPD_RESP_USE_STRLEN);
}
static esp_err_t web_root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req,"text/html; charset=utf-8");
    httpd_resp_set_hdr(req,"Cache-Control","no-store");
    return httpd_resp_send(req,WEB_HTML,HTTPD_RESP_USE_STRLEN);
}
static esp_err_t web_status_handler(httpd_req_t *req) {
    char response[JR_RESPONSE_MAX_BYTES];
    bool ok=jr_format_status(response,sizeof(response));
    return reply(req,ok?"200 OK":"500 Internal Server Error",ok?response:"JR_ERROR status_buffer");
}
static esp_err_t web_capture_handler(httpd_req_t *req) {
    jr_camera_jpeg_t frame = {0};
    esp_err_t err = jr_camera_capture_jpeg(&frame);
    if (err != ESP_OK) {
        char message[96];
        snprintf(message,sizeof(message),"JR_CAMERA_ERROR capture=%s",esp_err_to_name(err));
        return reply(req,"503 Service Unavailable",message);
    }

    char width[16],height[16],pid[16],bytes[24];
    snprintf(width,sizeof(width),"%u",frame.width);
    snprintf(height,sizeof(height),"%u",frame.height);
    snprintf(pid,sizeof(pid),"0x%04X",frame.pid);
    snprintf(bytes,sizeof(bytes),"%u",(unsigned)frame.len);
    httpd_resp_set_type(req,"image/jpeg");
    httpd_resp_set_hdr(req,"Cache-Control","no-store, no-cache, must-revalidate");
    httpd_resp_set_hdr(req,"X-JrBot-Camera","OV5640");
    httpd_resp_set_hdr(req,"X-JrBot-Width",width);
    httpd_resp_set_hdr(req,"X-JrBot-Height",height);
    httpd_resp_set_hdr(req,"X-JrBot-PID",pid);
    httpd_resp_set_hdr(req,"X-JrBot-Bytes",bytes);
    err = httpd_resp_send(req,(const char *)frame.data,(ssize_t)frame.len);
    if (err == ESP_OK) {
        ESP_LOGI("jrbot_portal","Foto OV5640 enviada %ux%u bytes=%u pid=0x%04X",
                 frame.width,frame.height,(unsigned)frame.len,frame.pid);
    } else {
        ESP_LOGW("jrbot_portal","Falha enviando foto OV5640: %s",esp_err_to_name(err));
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
    /* Provisioning/clearing credentials is deliberately Serial-only. */
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
void jr_portal_start(void) {
    if (web_server || !jr_wifi_network_ready()) return;
    httpd_config_t config=HTTPD_DEFAULT_CONFIG(); config.server_port=80; config.stack_size=12288;
    esp_err_t err=httpd_start(&web_server,&config);
    if (err!=ESP_OK) { web_server=NULL; ESP_LOGE("jrbot_portal","httpd_start=%s",esp_err_to_name(err)); return; }
    const httpd_uri_t routes[]={
        {.uri="/",.method=HTTP_GET,.handler=web_root_handler},
        {.uri="/status",.method=HTTP_GET,.handler=web_status_handler},
        {.uri="/cmd",.method=HTTP_POST,.handler=web_cmd_handler},
        {.uri="/cmd",.method=HTTP_GET,.handler=legacy_get_handler},
        {.uri="/capture",.method=HTTP_GET,.handler=web_capture_handler},
        {.uri="/autofocus",.method=HTTP_GET,.handler=disabled_camera_handler},
        {.uri="/manual-focus",.method=HTTP_GET,.handler=disabled_camera_handler}
    };
    for (size_t i=0;i<sizeof(routes)/sizeof(routes[0]);i++) {
        err=httpd_register_uri_handler(web_server,&routes[i]);
        if (err!=ESP_OK) { httpd_stop(web_server); web_server=NULL; ESP_LOGE("jrbot_portal","register=%s",esp_err_to_name(err)); return; }
    }
    ESP_LOGI("jrbot_portal","Portal local iniciado; foto JPEG em /capture; comandos POST; Wi-Fi config somente Serial");
}
