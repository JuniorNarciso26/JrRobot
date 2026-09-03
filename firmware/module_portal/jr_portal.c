#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>

#include "esp_http_server.h"
#include "esp_log.h"

#include "jr_config.h"
#include "jr_commands.h"
#include "jr_face.h"
#include "jr_wifi.h"
#include "jr_portal.h"

static const char *TAG = "jrbot_portal";
static httpd_handle_t web_server = NULL;

static const char WEB_HTML[] =
"<!doctype html><html lang='pt-br'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>JrBot</title><style>"
":root{--bg:#0b0d12;--panel:#151923;--line:#252b38;--txt:#eef3ff;--muted:#9aa7bd;--blue:#2f80ed;--green:#25a55f;--red:#d04b3f;--yellow:#f3b33d;--purple:#9b62f0}"
"*{box-sizing:border-box}body{margin:0;background:radial-gradient(circle at top,#182033,#0b0d12 48%);color:var(--txt);font-family:Inter,Segoe UI,Arial,sans-serif;min-height:100vh}"
"header{min-height:64px;display:flex;align-items:center;justify-content:space-between;gap:12px;padding:12px 22px;border-bottom:1px solid var(--line);background:rgba(10,12,18,.75)}"
"h1{font-size:20px;margin:0}.version{color:#b9ffd7;font-size:12px;margin-top:4px}.sub{color:var(--muted);font-size:13px;margin-top:3px}.status{display:flex;gap:10px;align-items:center;flex-wrap:wrap}.pill{padding:8px 12px;border:1px solid var(--line);border-radius:999px;background:var(--panel);font-size:13px;color:var(--muted)}.pill.ok{color:#b9ffd7;border-color:#246b43}"
"main{display:grid;grid-template-columns:1fr 1fr;gap:14px;padding:14px}.card{background:rgba(21,25,35,.88);border:1px solid var(--line);border-radius:18px;overflow:hidden;box-shadow:0 12px 30px rgba(0,0,0,.25)}.card h2{font-size:15px;margin:0;padding:14px 16px;border-bottom:1px solid var(--line);color:#dfe8ff;display:flex;justify-content:space-between;align-items:center}"
"button{border:0;border-radius:12px;padding:10px 13px;color:white;font-weight:700;cursor:pointer;background:var(--blue);margin:0}.green{background:var(--green)}.red{background:var(--red)}.gray{background:#30394d}.yellow{background:var(--yellow);color:#1c1400}.purple{background:var(--purple)}input{background:#090b10;color:var(--txt);border:1px solid var(--line);border-radius:10px;padding:10px;font-size:14px}"
"#log{min-height:360px;margin:0;padding:14px;background:#050609;color:#a8ffbf;font-family:Consolas,Menlo,monospace;font-size:13px;line-height:1.35;overflow:auto;white-space:pre-wrap}.logline .ts{color:#6e7890}.logline .tx{color:#7ab7ff}.logline .err{color:#ff8f86}.logline .ok{color:#a8ffbf}.hint{color:var(--muted);font-size:13px;padding:12px}"
".faces{padding:14px;display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}.face{display:flex;align-items:center;gap:10px;text-align:left;background:linear-gradient(180deg,#202739,#161b27);border:1px solid #30384b;padding:12px;border-radius:14px;min-height:68px}.face .emoji{font-size:25px;width:34px;text-align:center}.face .name{font-size:15px}.face .cmd{font-size:12px;color:var(--muted);margin-top:2px}.quick{padding:12px;border-top:1px solid var(--line);display:flex;gap:8px;flex-wrap:wrap}.custom{display:flex;gap:8px;width:100%}.custom input{flex:1}"
"@media(max-width:850px){main{grid-template-columns:1fr}.faces{grid-template-columns:1fr}#log{min-height:300px}}"
"</style></head><body>"
"<header><div><h1>JrBot</h1><div class='version'>Versao: " JR_APP_VERSION "</div><div class='sub'>Painel Wi-Fi direto no ESP32 | mesma base visual do painel Serial</div></div><div class='status'><span id='conn' class='pill ok'>Wi-Fi</span><span id='last' class='pill'>sem log</span></div></header>"
"<main><section class='card'><h2>Status e retorno <span><button class='gray' onclick='clearLog()'>limpar</button></span></h2><pre id='log'></pre><div class='hint'>Se travar, volte ao PAINEL.bat e escolha Serial USB para diagnosticar.</div></section>"
"<section class='card'><h2>Rostos</h2><div class='faces' id='faces'></div><div class='quick'><button onclick=\"send('status')\" class='gray'>status</button><button onclick=\"send('help')\" class='gray'>help</button><button onclick=\"send('demo')\" class='purple'>demo on/off</button><div class='custom'><input id='cmd' placeholder='comando manual'><button onclick='sendCustom()'>enviar</button></div></div></section></main>"
"<script>const faces=[['🤖','Neutro','neutro'],['😊','Feliz','feliz'],['😢','Triste','triste'],['😃','Animado','animado'],['😠','Bravo','bravo'],['😮','Surpreso','surpreso'],['🤔','Pensando','pensando'],['😒','Cetico','cetico'],['😴','Sono','sono'],['😵💫','Confuso','confuso'],['😉','Piscando','piscando'],['😍','Amor','amor'],['😜','Brincalhao','brincalhao'],['😟','Preocupado','preocupado'],['😎','Cool','cool'],['🔋','Bateria baixa','bateria']];"
"let last='';const logEl=document.getElementById('log');document.getElementById('faces').innerHTML=faces.map(f=>`<button class='face' onclick=\"send('${f[2]}')\"><span class='emoji'>${f[0]}</span><span><div class='name'>${f[1]}</div><div class='cmd'>${f[2]}</div></span></button>`).join('');"
"function now(){return new Date().toLocaleTimeString()}function log(t,tx){let cls=(t.includes('ERROR')||t.includes('erro'))?'err':(tx?'tx':'ok');logEl.innerHTML=`<div class='logline'><span class='ts'>[${now()}]</span> <span class='${cls}'></span></div>`+logEl.innerHTML;logEl.querySelector('span:last-child').textContent=t;document.getElementById('last').textContent=now()}function clearLog(){logEl.textContent=''}"
"async function send(c){c=(c||'').trim();if(!c)return;log('> '+c,true);let r=await fetch('/cmd?c='+encodeURIComponent(c));let t=await r.text();log(t,false);status()}function sendCustom(){send(document.getElementById('cmd').value)}document.getElementById('cmd').addEventListener('keydown',e=>{if(e.key==='Enter')sendCustom()});"
"async function status(){try{let r=await fetch('/status');let t=await r.text();if(t!==last){last=t;log(t,false)}}catch(e){log('JR_WIFI_ERROR '+e.message,false)}}status();setInterval(status,3000)</script></body></html>";

static esp_err_t web_root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, WEB_HTML, HTTPD_RESP_USE_STRLEN);
}
static esp_err_t web_status_handler(httpd_req_t *req) {
    char body[256];
    snprintf(body, sizeof(body), "JR_STATUS v=4 version=%s expression=%s demo=%d wifi=1 ssid=%s ip=%s commands=%lu frames=%lu",
        JR_APP_VERSION, jr_face_expression_name(), jr_face_demo_enabled() ? 1 : 0, jr_wifi_ssid(), jr_wifi_ip(), (unsigned long)jr_face_command_count(), (unsigned long)jr_face_frame_count());
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    return httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
}
static esp_err_t web_cmd_handler(httpd_req_t *req) {
    char query[128] = {0}; char cmd[64] = {0}; char response[256] = {0};
    httpd_req_get_url_query_str(req, query, sizeof(query));
    httpd_query_key_value(query, "c", cmd, sizeof(cmd));
    for (size_t i=0; cmd[i]; ++i) cmd[i] = (char)tolower((unsigned char)cmd[i]);
    bool ok = cmd[0] && jr_handle_command(cmd, response, sizeof(response));
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    return httpd_resp_send(req, response[0] ? response : (ok ? "ok" : "comando vazio"), HTTPD_RESP_USE_STRLEN);
}
void jr_portal_start(void) {
    if (web_server) return;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    if (httpd_start(&web_server, &config) == ESP_OK) {
        httpd_uri_t root = {.uri="/", .method=HTTP_GET, .handler=web_root_handler};
        httpd_uri_t status = {.uri="/status", .method=HTTP_GET, .handler=web_status_handler};
        httpd_uri_t cmd = {.uri="/cmd", .method=HTTP_GET, .handler=web_cmd_handler};
        httpd_register_uri_handler(web_server, &root);
        httpd_register_uri_handler(web_server, &status);
        httpd_register_uri_handler(web_server, &cmd);
        ESP_LOGI(TAG, "Servidor web iniciado. Abra http://IP_DO_ESP32/ quando Wi-Fi conectar.");
    }
}
