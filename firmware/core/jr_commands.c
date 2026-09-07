#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "jr_config.h"
#include "jr_board.h"
#include "jr_commands.h"
#include "jr_audio.h"
#include "jr_mic.h"
#include "jr_wifi.h"
#include "jr_face.h"
#include "jr_camera_diag.h"
#include "jr_brain.h"

static SemaphoreHandle_t command_lock;
static bool terminal_started;

esp_err_t jr_commands_init(void) {
    if (jr_audio_bus_init() != ESP_OK) return ESP_ERR_NO_MEM;
    if (!command_lock) command_lock = xSemaphoreCreateMutex();
    return command_lock ? ESP_OK : ESP_ERR_NO_MEM;
}
static int hex_value(unsigned char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
bool jr_decode_component(const char *src, size_t len, char *dst, size_t cap, bool plus_space) {
    if (!src || !dst || !cap) return false;
    size_t n = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)src[i];
        if (c == '%') {
            if (i + 2 >= len) return false;
            int h = hex_value((unsigned char)src[i+1]), l = hex_value((unsigned char)src[i+2]);
            if (h < 0 || l < 0) return false;
            c = (unsigned char)((h << 4) | l); i += 2;
        } else if (c == '+' && plus_space) c = ' ';
        if (c < 32 || c == 127 || n + 1 >= cap) return false;
        dst[n++] = (char)c;
    }
    dst[n] = '\0';
    return true;
}

static bool parse_wifi_config(const char *args, bool encoded, char *response, size_t cap) {
    char ssid[33] = "", pass[65] = "", host[33] = "jrbot";
    char ip[16] = "192.168.0.50", gw[16] = "192.168.0.1", mask[16] = "255.255.255.0";
    char dns1[16] = "8.8.8.8", dns2[16] = "8.8.4.4", stat[6] = "0";
    struct field { const char *key; char *value; size_t size; } fields[] = {
        {"ssid",ssid,sizeof(ssid)}, {"pass",pass,sizeof(pass)}, {"host",host,sizeof(host)},
        {"static",stat,sizeof(stat)}, {"ip",ip,sizeof(ip)}, {"gw",gw,sizeof(gw)},
        {"mask",mask,sizeof(mask)}, {"dns1",dns1,sizeof(dns1)}, {"dns2",dns2,sizeof(dns2)}
    };
    unsigned seen = 0;
    const char *p = args;
    while (*p) {
        const char *end = strchr(p, '|');
        size_t len = end ? (size_t)(end - p) : strlen(p);
        const char *eq = memchr(p, '=', len);
        if (!eq) goto invalid;
        size_t klen = (size_t)(eq - p), vlen = len - klen - 1;
        size_t i;
        for (i = 0; i < sizeof(fields)/sizeof(fields[0]); i++)
            if (strlen(fields[i].key) == klen && !memcmp(p, fields[i].key, klen)) break;
        if (i == sizeof(fields)/sizeof(fields[0]) || (seen & (1U << i))) goto invalid;
        seen |= 1U << i;
        if (encoded) {
            if (!jr_decode_component(eq+1, vlen, fields[i].value, fields[i].size, false)) goto invalid;
        } else {
            if (vlen >= fields[i].size) goto invalid;
            memcpy(fields[i].value, eq+1, vlen); fields[i].value[vlen] = '\0';
        }
        if (!end) break;
        p = end + 1;
        if (!*p) goto invalid;
    }
    if (!(seen & 1) || (strcmp(stat,"0") && strcmp(stat,"1"))) goto invalid;
    {
        bool ok = jr_wifi_configure(ssid,pass,host,!strcmp(stat,"1"),ip,gw,mask,dns1,dns2,response,(unsigned)cap);
        memset(pass, 0, sizeof(pass));
        return ok;
    }
invalid:
    memset(pass, 0, sizeof(pass));
    snprintf(response, cap, "JR_ERROR wifi_campos_invalidos_ou_longos");
    return false;
}

bool jr_format_status(char *response, size_t cap) {
    jr_face_status_t f;
    jr_wifi_status_t w;
    jr_mic_status_t m = {0};
    jr_mic_recording_info_t mr = {0};
    jr_audio_diag_t a = {0};
    jr_face_get_status(&f);
    jr_wifi_get_status(&w);
    jr_audio_get_diag(&a);
    jr_mic_get_recording_info(&mr);
    bool oled_present = (f.state == JR_OLED_READY || f.state == JR_OLED_DEGRADED);
    unsigned camera_pid = 0;
    bool camera_present = jr_camera_probe_once(&camera_pid);
    bool mic_present = jr_mic_probe(&m);
    const char *camera_state = camera_present ? "available" : "unavailable";
    const char *mic_state = mic_present ? "available" : "unavailable";
    const char *audio_last = a.tests ? esp_err_to_name(a.last_error) : "not_run";
    int n = snprintf(response, cap,
        "JR_STATUS protocol=2 version=%s build_sp=%s hardware=%s profile=%s expression=%s demo=%d "
        "wifi_config=%d wifi=%d pending_restart=%d ip=%s audio=on_demand volume=%d "
        "audio_bclk=%d audio_ws=%d audio_dout=%d amplifier_presence=not_detectable "
        "audio_test_seq=%lu audio_test_running=%d audio_last=%s audio_last_bytes=%lu "
        "camera=%s camera_model=%s camera_pid=0x%04X "
        "mic=%s mic_model=%s mic_sck=%d mic_ws=%d mic_sd=%d mic_channel=%c mic_samples=%u mic_peak_raw=%lu mic_changes=%u "
        "mic_recording=%d mic_has_recording=%d mic_record_seconds=%u mic_record_samples=%u mic_record_level=%d mic_record_peak=%d "
        "oled=%s oled_presence=%s oled_addr=0x%02X sda=1 scl=2 hz=%d commands=%lu rendered=%lu tx_ok=%lu "
        "tx_fail=%lu skipped=%lu init_fail=%lu consecutive_fail=%lu recoveries=%lu last_success_ms=%lu last_error=%s",
        JR_APP_VERSION,JR_BUILD_STAMP_SP,JR_PINMAP_REVISION,JR_PROFILE_NAME,f.expression,f.demo,
        w.configured,w.connected,w.pending_restart,w.ip,jr_audio_volume(),
        JR_AUDIO_BCLK_GPIO,JR_AUDIO_LRC_GPIO,JR_AUDIO_DIN_GPIO,
        (unsigned long)a.tests,a.running?1:0,audio_last,(unsigned long)a.last_bytes,
        camera_state,JR_CAMERA_MODEL,camera_pid,
        mic_state,JR_MIC_MODEL,JR_MIC_SCK_GPIO,JR_MIC_WS_GPIO,JR_MIC_SD_GPIO,m.channel?m.channel:'L',m.samples,(unsigned long)m.peak_raw,m.changes,
        mr.recording?1:0,mr.has_recording?1:0,mr.seconds,(unsigned)mr.samples,mr.level,mr.peak,
        jr_face_state_name(f.state),oled_present?"available":"unavailable",f.address,JR_OLED_I2C_HZ,
        (unsigned long)f.commands,(unsigned long)f.rendered,(unsigned long)f.tx_ok,
        (unsigned long)f.tx_failed,(unsigned long)f.skipped,(unsigned long)f.init_failed,
        (unsigned long)f.consecutive_failures,(unsigned long)f.recoveries,
        (unsigned long)f.last_success_ms,esp_err_to_name(f.last_error));
    return n >= 0 && (size_t)n < cap;
}

static bool execute_command(const char *cmd, char *response, size_t cap) {
    size_t len = strlen(cmd);
    if (!len || len > JR_COMMAND_MAX_BYTES) goto invalid;
    for (size_t i=0; i<len; i++) if ((unsigned char)cmd[i] < 32 || (unsigned char)cmd[i] == 127) goto invalid;
    const char *p = cmd;
    while (*p == ' ') p++;
    const char *space = strchr(p, ' ');
    size_t verb_len = space ? (size_t)(space-p) : strlen(p);
    char verb[32];
    if (!verb_len || verb_len >= sizeof(verb)) goto invalid;
    for (size_t i=0; i<verb_len; i++) verb[i] = (char)tolower((unsigned char)p[i]);
    verb[verb_len] = '\0';
    const char *args = space ? space+1 : "";
    if (!strcmp(verb,"wifi_config") || !strcmp(verb,"wifi_config_pct"))
        return parse_wifi_config(args,!strcmp(verb,"wifi_config_pct"),response,cap);
    if (!strcmp(verb,"audio_volume") || !strcmp(verb,"volume")) {
        char *end; errno=0;
        long value = strtol(args,&end,10);
        if (!*args || end == args || errno || value<0 || value>100) goto invalid;
        while (*end==' ') end++;
        if (*end) goto invalid;
        jr_audio_set_volume((int)value);
        snprintf(response,cap,"JR_OK audio_volume=%d audio=enabled",jr_audio_volume());
        return true;
    }
    while (*args==' ') args++;
    if (*args) goto invalid;
    if (!strcmp(verb,"autonomo_on")) {
        esp_err_t err = jr_brain_set_enabled(true);
        snprintf(response,cap,err==ESP_OK ? "JR_OK autonomous=1 brain=listening" : "JR_ERROR autonomous_on=%s",esp_err_to_name(err));
        return err==ESP_OK;
    }
    if (!strcmp(verb,"autonomo_off")) {
        esp_err_t err = jr_brain_set_enabled(false);
        snprintf(response,cap,err==ESP_OK ? "JR_OK autonomous=0 brain=off" : "JR_ERROR autonomous_off=%s",esp_err_to_name(err));
        return err==ESP_OK;
    }
    if (!strcmp(verb,"brain_status")) {
        jr_brain_status_t b = {0};
        jr_brain_get_status(&b);
        snprintf(response,cap,"JR_OK brain_state=%s enabled=%d listening=%d engine=%s triggers=%lu last_event=%s last_error=%s",
                 jr_brain_state_name(b.state),b.enabled?1:0,b.listening?1:0,b.engine,
                 (unsigned long)b.triggers,b.last_event,esp_err_to_name(b.last_error));
        return true;
    }
    if (!strcmp(verb,"brain_test")) {
        esp_err_t err = jr_brain_trigger_test();
        if (err==ESP_OK) {
            jr_brain_status_t b = {0};
            jr_brain_get_status(&b);
            snprintf(response,cap,"JR_OK brain_test=wakeword_detected expression=%s triggers=%lu",
                     jr_face_expression_name(),(unsigned long)b.triggers);
            return true;
        }
        snprintf(response,cap,"JR_ERROR brain_test=%s hint=ative_autonomo_primeiro",esp_err_to_name(err));
        return false;
    }
    if (!strcmp(verb,"status")) return jr_format_status(response,cap);
    if (!strcmp(verb,"version")) {
        snprintf(response,cap,"JR_OK version=%s build_sp=%s hardware=%s profile=%s",JR_APP_VERSION,JR_BUILD_STAMP_SP,JR_PINMAP_REVISION,JR_PROFILE_NAME);
        return true;
    }
    if (!strcmp(verb,"camera_test")) return jr_camera_test_once(response,cap);
    if (!strcmp(verb,"mic_test")) return jr_mic_test(response,cap);
    if (!strcmp(verb,"mic_status")) return jr_mic_status(response,cap);
    if (!strcmp(verb,"audio_play_recording") || !strcmp(verb,"play_recording") || !strcmp(verb,"tocar_gravacao")) {
        jr_mic_recording_info_t info;
        jr_mic_get_recording_info(&info);
        if (!info.has_recording) {
            snprintf(response,cap,"JR_ERROR audio_play_recording=no_recording");
            return false;
        }
        esp_err_t err = jr_mic_play_last_recording();
        if (err == ESP_OK) {
            snprintf(response,cap,"JR_OK audio_play_recording=completed samples=%u seconds=%u volume=%d",
                     (unsigned)info.samples,info.seconds,jr_audio_volume());
            return true;
        }
        snprintf(response,cap,"JR_ERROR audio_play_recording=%s samples=%u",
                 esp_err_to_name(err),(unsigned)info.samples);
        return false;
    }
    if (!strcmp(verb,"audio_diag")) {
        jr_audio_diag_t a;
        jr_audio_get_diag(&a);
        snprintf(response,cap,"JR_OK audio_diag seq=%lu running=%d last=%s bytes=%lu freq=%d duration_ms=%d volume=%d",
                 (unsigned long)a.tests,a.running?1:0,a.tests?esp_err_to_name(a.last_error):"not_run",
                 (unsigned long)a.last_bytes,a.last_frequency_hz,a.last_duration_ms,jr_audio_volume());
        return true;
    }
    if (!strcmp(verb,"help") || !strcmp(verb,"ajuda")) {
        snprintf(response,cap,"JR_HELP protocol=2 version status autonomo_on autonomo_off brain_status brain_test camera_test mic_test mic_status audio_test audio_play_recording audio_diag demo neutro feliz triste animado bravo surpreso pensando cetico sono confuso piscando amor brincalhao preocupado cool bateria audio_volume[0-100] wifi_config_pct wifi_clear"); return true;
    }
    if (!strcmp(verb,"wifi_clear")) return jr_wifi_clear(response,(unsigned)cap);
    if (!strcmp(verb,"demo")) {
        jr_face_set_demo(!jr_face_demo_enabled());
        snprintf(response,cap,"JR_OK demo=%d",jr_face_demo_enabled()); return true;
    }
    if (!strcmp(verb,"audio_test") || !strcmp(verb,"som") || !strcmp(verb,"beep")) {
        esp_err_t err=jr_audio_test_tone(880,900);
        jr_audio_diag_t a;
        jr_audio_get_diag(&a);
        if (err==ESP_OK) {
            snprintf(response,cap,"JR_OK audio_test=tx_completed seq=%lu bytes=%lu freq=%d duration_ms=%d audible_check=pending",
                     (unsigned long)a.tests,(unsigned long)a.last_bytes,a.last_frequency_hz,a.last_duration_ms);
        } else {
            snprintf(response,cap,"JR_ERROR audio_test=%s seq=%lu bytes=%lu",
                     esp_err_to_name(err),(unsigned long)a.tests,(unsigned long)a.last_bytes);
        }
        return err==ESP_OK;
    }
    if (jr_face_set_expression(verb)) {
        jr_face_increment_command_count();
        snprintf(response,cap,"JR_OK expression=%s",jr_face_expression_name()); return true;
    }
invalid:
    snprintf(response,cap,"JR_ERROR comando_ou_argumentos_invalidos"); return false;
}

bool jr_handle_command(const char *cmd, char *response, size_t cap) {
    if (!response || cap < JR_RESPONSE_MAX_BYTES) return false;
    response[0]='\0';
    if (!cmd) { snprintf(response,cap,"JR_ERROR comando_vazio"); return false; }
    if (!command_lock || xSemaphoreTake(command_lock,pdMS_TO_TICKS(1500))!=pdTRUE) {
        snprintf(response,cap,"JR_ERROR commands_busy_or_unavailable"); return false;
    }
    bool ok=execute_command(cmd,response,cap);
    xSemaphoreGive(command_lock);
    return ok;
}

void jr_print_help(void) { printf("JR_READY protocol=2 max_command_bytes=%d; use help\n",JR_COMMAND_MAX_BYTES); }

void jr_terminal_process_line(const char *line) {
    char id[33]="";
    const char *cmd=line;
    if (*cmd=='@') {
        const char *space=strchr(cmd,' ');
        if (!space || space-cmd<2 || space-cmd>33) { puts("JR_ERROR invalid_id"); return; }
        size_t n=(size_t)(space-cmd-1);
        for (size_t i=0;i<n;i++) {
            unsigned char c=(unsigned char)cmd[i+1];
            if (!isalnum(c) && c!='-' && c!='_') { puts("JR_ERROR invalid_id"); return; }
        }
        memcpy(id,cmd+1,n); cmd=space+1;
    }
    char response[JR_RESPONSE_MAX_BYTES];
    bool ok=jr_handle_command(cmd,response,sizeof(response));
    if (*id) printf("JR_REPLY id=%s ok=%d %s\n",id,ok?1:0,response);
    else printf("%s\n",response);
}

typedef struct { char line[JR_SERIAL_LINE_MAX_BYTES+1]; size_t size; bool discard; } serial_parser_t;
static unsigned long serial_noise_lines;
static TickType_t serial_noise_last_report;

static void serial_noise_drop(void) {
    serial_noise_lines++;
    TickType_t now=xTaskGetTickCount();
    if ((TickType_t)(now-serial_noise_last_report) >= pdMS_TO_TICKS(2000)) {
        printf("JR_WARN serial_noise dropped_lines=%lu parser_resynced=1\n",serial_noise_lines);
        serial_noise_lines=0;
        serial_noise_last_report=now;
    }
}

static void terminal_feed(serial_parser_t *s, const uint8_t *data, size_t len) {
    for (size_t i=0;i<len;i++) {
        unsigned char c=data[i];
        if (c=='\r' || c=='\n') {
            if (s->discard) serial_noise_drop();
            else if (s->size) {
                s->line[s->size]='\0';
                unsigned char first=(unsigned char)s->line[0];
                if (first=='@' || isalpha(first)) jr_terminal_process_line(s->line);
                else serial_noise_drop();
            }
            s->size=0;
            s->discard=false;
        } else if (!s->discard) {
            if (c<32 || c==127) {
                if (s->size) s->discard=true;
            } else if (s->size>=JR_SERIAL_LINE_MAX_BYTES) {
                s->discard=true;
            } else {
                s->line[s->size++]=(char)c;
            }
        }
    }
}
static void terminal_task(void *arg) {
    (void)arg;
    uint8_t rx[256]; serial_parser_t parser={0}; jr_print_help();
    (void)uart_flush_input(UART_NUM_0);
    for (;;) {
        int n=uart_read_bytes(UART_NUM_0,rx,sizeof(rx),pdMS_TO_TICKS(100));
        if (n>0) terminal_feed(&parser,rx,(size_t)n);
    }
}
void jr_terminal_start(void) {
    if (terminal_started) return;
    esp_err_t err=uart_driver_install(UART_NUM_0,8192,0,0,NULL,0);
    if (err!=ESP_OK && err!=ESP_ERR_INVALID_STATE) { printf("JR_ERROR terminal_uart=%s\n",esp_err_to_name(err)); return; }
    (void)uart_set_rx_timeout(UART_NUM_0,2);
    if (xTaskCreate(terminal_task,"terminal",8192,NULL,5,NULL)!=pdPASS) { puts("JR_ERROR terminal_task_create"); return; }
    terminal_started=true;
}
