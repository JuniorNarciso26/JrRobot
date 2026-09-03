#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>

#include "esp_http_server.h"
#include "esp_log.h"

#include "jr_config.h"
#include "jr_camera.h"
#include "jr_commands.h"
#include "jr_face.h"
#include "jr_wifi.h"
#include "jr_portal.h"

static const char *TAG = "jrbot_portal";
static httpd_handle_t web_server = NULL;

static const char WEB_HTML[] =
"<!doctype html><html lang='pt-br'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>JrBot camera + OLED</title><style>body{margin:0;background:#0b0d12;color:#eef3ff;font-family:Arial,sans-serif;padding:18px}.wrap{max-width:980px;margin:auto}.card{background:#151923;border:1px solid #252b38;border-radius:16px;padding:18px;margin:12px 0}button{padding:12px 14px;margin:6px;border:0;border-radius:10px;background:#2f80ed;color:#fff;font-weight:700}.muted{color:#9aa7bd}.ok{color:#b9ffd7}.warn{color:#f3b33d}input{padding:12px;border-radius:10px;border:1px solid #252b38;background:#050609;color:#fff;min-width:220px}img{width:100%;max-width:900px;border-radius:12px;background:#050609;margin-top:12px}.row{display:flex;gap:8px;align-items:center;flex-wrap:wrap}input[type=range]{min-width:260px}</style></head>"
"<body><div class='wrap'><div class='card'><h1>JrBot camera + OLED</h1><div class='ok'>Versao: " JR_APP_VERSION "</div><p class='muted'>Portal direto no ESP32: câmera OV5640 + comandos do rosto OLED.</p><div id='status'>carregando...</div></div><div class='card'><h2>Camera</h2><div class='row'><button onclick='capture(1)'>Focar e tirar foto</button><button onclick='capture(0)'>Tirar foto sem foco</button><button onclick='autofocus()'>Ajustar foco</button></div><div class='row'><label>Foco manual <input id='focus' type='range' min='0' max='100' value='50' oninput='focusValue.innerText=this.value+\"%\"'></label><span id='focusValue'>50%</span><button onclick='manualFocus()'>Aplicar</button><button onclick='manualFocusAndCapture()'>Aplicar e fotografar</button></div><p id='cammsg' class='warn'></p><img id='photo' alt='Foto da camera'></div><div class='card'><h2>Rostos OLED</h2><div id='faces'></div></div><div class='card'><input id='cmd' placeholder='comando manual'><button onclick='send(document.getElementById(\"cmd\").value)'>Enviar</button></div><div class='card'><pre id='log'></pre></div></div>"
"<script>const faces=['neutro','feliz','triste','animado','bravo','surpreso','pensando','cetico','sono','confuso','piscando','amor','brincalhao','preocupado','cool','bateria','demo','status'];const msg=document.getElementById('cammsg'),photo=document.getElementById('photo'),focus=document.getElementById('focus');document.getElementById('faces').innerHTML=faces.map(c=>`<button onclick=send('${c}')>${c}</button>`).join('');async function send(c){let r=await fetch('/cmd?c='+encodeURIComponent((c||'').trim()));let t=await r.text();log(t);status()}function log(t){document.getElementById('log').textContent='['+new Date().toLocaleTimeString()+'] '+t+'\\n'+document.getElementById('log').textContent}async function status(){let r=await fetch('/status');document.getElementById('status').textContent=await r.text()}function capture(f){msg.innerText=f?'Focando e fotografando...':'Fotografando...';photo.onload=()=>msg.innerText='Foto atualizada.';photo.onerror=()=>msg.innerText='Erro ao tirar foto.';photo.src='/capture?focus='+f+'&t='+Date.now()}function autofocus(){msg.innerText='Ajustando foco...';fetch('/autofocus').then(r=>r.text()).then(t=>{msg.innerText=t;setTimeout(()=>capture(0),500)}).catch(()=>msg.innerText='Erro ao ajustar foco.')}function manualFocus(){msg.innerText='Aplicando foco '+focus.value+'%...';return fetch('/manual-focus?value='+focus.value).then(r=>r.text()).then(t=>{msg.innerText=t;return t}).catch(()=>msg.innerText='Erro no foco manual.')}function manualFocusAndCapture(){manualFocus().then(()=>setTimeout(()=>capture(0),350))}status();setInterval(status,3000);capture(0)</script></body></html>";

static esp_err_t web_root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, WEB_HTML, HTTPD_RESP_USE_STRLEN);
}
static esp_err_t web_status_handler(httpd_req_t *req) {
    char body[256];
    snprintf(body, sizeof(body), "Versao: %s | rosto: %s | demo: %d | ip: %s | camera: %s | comandos: %lu | frames: %lu",
        JR_APP_VERSION, jr_face_expression_name(), jr_face_demo_enabled() ? 1 : 0, jr_wifi_ip(), jr_camera_status_text(), (unsigned long)jr_face_command_count(), (unsigned long)jr_face_frame_count());
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    return httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
}
static esp_err_t web_cmd_handler(httpd_req_t *req) {
    char query[96] = {0}; char cmd[32] = {0}; char response[192] = {0};
    httpd_req_get_url_query_str(req, query, sizeof(query));
    httpd_query_key_value(query, "c", cmd, sizeof(cmd));
    for (size_t i=0; cmd[i]; ++i) cmd[i] = (char)tolower((unsigned char)cmd[i]);
    bool ok = cmd[0] && jr_handle_command(cmd, response, sizeof(response));
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    return httpd_resp_send(req, response[0] ? response : (ok ? "ok" : "comando vazio"), HTTPD_RESP_USE_STRLEN);
}
void jr_portal_start(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    if (httpd_start(&web_server, &config) == ESP_OK) {
        httpd_uri_t root = {.uri="/", .method=HTTP_GET, .handler=web_root_handler};
        httpd_uri_t status = {.uri="/status", .method=HTTP_GET, .handler=web_status_handler};
        httpd_uri_t cmd = {.uri="/cmd", .method=HTTP_GET, .handler=web_cmd_handler};
        httpd_uri_t capture = {.uri="/capture", .method=HTTP_GET, .handler=jr_camera_capture_handler};
        httpd_uri_t autofocus = {.uri="/autofocus", .method=HTTP_GET, .handler=jr_camera_autofocus_handler};
        httpd_uri_t manual_focus = {.uri="/manual-focus", .method=HTTP_GET, .handler=jr_camera_manual_focus_handler};
        httpd_register_uri_handler(web_server, &root);
        httpd_register_uri_handler(web_server, &status);
        httpd_register_uri_handler(web_server, &cmd);
        httpd_register_uri_handler(web_server, &capture);
        httpd_register_uri_handler(web_server, &autofocus);
        httpd_register_uri_handler(web_server, &manual_focus);
        ESP_LOGI(TAG, "Servidor web iniciado. Abra http://IP_DO_ESP32/ quando Wi-Fi conectar.");
    }
}
