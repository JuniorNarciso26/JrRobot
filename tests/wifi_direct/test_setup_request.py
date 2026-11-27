"""Exercise the real setup form parser and direct-enable HTTP handler on host."""
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
def function(source,name):
    match=re.search(r'static (?:int|bool|esp_err_t) '+name+r'\([^)]*\)\s*\{',source)
    assert match,name
    depth=0
    for i in range(match.end()-1,len(source)):
        if source[i]=='{': depth+=1
        elif source[i]=='}':
            depth-=1
            if not depth:return source[match.start():i+1]
    raise AssertionError(name)
PRE=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_INVALID_ARG 2
#define HTTPD_403_FORBIDDEN 403
#define HTTPD_400_BAD_REQUEST 400
#define JR_MODE_IDLE 0
#define portENTER_CRITICAL(x) ((void)(x))
#define portEXIT_CRITICAL(x) ((void)(x))
static int setup_test_lock, mode, status, configured;
static bool busy, direct_restart_pending, marker=true, bad_type;
static uint32_t direct_restart_ms;
static char saved[65];
typedef struct { int content_len; const char *body; int offset; bool broken; } httpd_req_t;
static bool setup_test_busy(void) { return busy; }
static int jr_mode_get(void) { return mode; }
static uint32_t setup_now_ms(void) { return 1234; }
static esp_err_t httpd_req_get_hdr_value_str(httpd_req_t *r,const char *key,char *out,size_t n) {
    (void)r;
    if(!strcmp(key,"X-JrBot-WiFi")&&!marker)return ESP_FAIL;
    snprintf(out,n,"%s",!strcmp(key,"X-JrBot-WiFi")?"1":bad_type?"text/plain":"application/x-www-form-urlencoded");return ESP_OK;
}
static esp_err_t httpd_resp_send_err(httpd_req_t *r,int code,const char *message) { (void)r;(void)message;status=code;return ESP_OK; }
static int httpd_req_recv(httpd_req_t *r,char *out,size_t n) {
    if(r->broken)return -1;
    if(n>3)n=3;memcpy(out,r->body+r->offset,n);r->offset+=n;return n;
}
static esp_err_t jr_wifi_direct_configure(bool enabled,const char *password) {
    assert(enabled);size_t n=strlen(password);if(n<8||n>63)return ESP_ERR_INVALID_ARG;
    for(size_t i=0;i<n;i++)if((unsigned char)password[i]>126)return ESP_ERR_INVALID_ARG;
    configured++;strcpy(saved,password);return ESP_OK;
}
static void httpd_resp_set_type(httpd_req_t *r,const char *v) { (void)r;(void)v; }
static void httpd_resp_set_hdr(httpd_req_t *r,const char *k,const char *v) { (void)r;(void)k;(void)v; }
static esp_err_t httpd_resp_sendstr(httpd_req_t *r,const char *v) { (void)r;assert(strstr(v,"restarting"));status=200;return ESP_OK; }
'''
POST=r'''
static void request(const char *body) {
    httpd_req_t req={.body=body,.content_len=(int)strlen(body)};
    status=configured=0;direct_restart_pending=false;
    assert(setup_direct_start(&req)==ESP_OK);
}
int main(void) {
    marker=false;request("ssid=direct&pass=secret123");assert(status==403&&!configured);
    marker=true;bad_type=true;request("ssid=direct&pass=secret123");assert(status==403&&!configured);
    bad_type=false;busy=true;request("ssid=direct&pass=secret123");assert(status==400&&!configured);
    busy=false;mode=1;request("ssid=direct&pass=secret123");assert(status==400&&!configured);mode=0;
    request("ssid=direct&pass=short");assert(status==400&&!configured&&!direct_restart_pending);
    request("ssid=direct&pass=secret%00suffix");assert(status==400&&!configured);
    request("ssid=direct&pass=secret%ZZsuffix");assert(status==400&&!configured);
    request("ssid=direct&pass=secret123&pass=duplicate");assert(status==400&&!configured);
    request("ssid=direct&pass=secret%2B12+3");
    assert(status==200&&configured==1&&direct_restart_pending&&direct_restart_ms==1234);
    assert(!strcmp(saved,"secret+12 3"));
    return 0;
}
'''
class SetupRequestTests(unittest.TestCase):
    def test_auth_busy_validation_and_fragmented_body(self):
        source=(ROOT/'firmware/core/jr_wifi_provisioning.c').read_text()
        functions='\n'.join(function(source,n) for n in ('hex_value','decode_value','parse_form','setup_direct_start'))
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp);(p/'request.c').write_text(PRE+functions+POST)
            subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',str(p/'request.c'),'-o',str(p/'request')],check=True)
            subprocess.run([str(p/'request')],check=True)
