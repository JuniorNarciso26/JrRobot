#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "jr_config.h"
#include "jr_commands.h"
#include "jr_wifi.h"
#include "jr_portal.h"
#include "jr_face.h"

static void lower_copy(char *dst, size_t dst_len, const char *src) {
    size_t i = 0;
    if (dst_len == 0) return;
    for (; src && src[i] && i < dst_len - 1; ++i) dst[i] = (char)tolower((unsigned char)src[i]);
    dst[i] = '\0';
}

static void field_copy(char *dst, size_t dst_len, const char *src) {
    if (dst_len == 0) return;
    snprintf(dst, dst_len, "%s", src ? src : "");
}

static void parse_wifi_config(const char *cmd, char *ssid, size_t ssid_len, char *pass, size_t pass_len,
                              char *host, size_t host_len, bool *use_static, char *ip, size_t ip_len,
                              char *gw, size_t gw_len, char *mask, size_t mask_len,
                              char *dns1, size_t dns1_len, char *dns2, size_t dns2_len) {
    field_copy(host, host_len, "jrbot");
    field_copy(ip, ip_len, "192.168.0.50");
    field_copy(gw, gw_len, "192.168.0.1");
    field_copy(mask, mask_len, "255.255.255.0");
    field_copy(dns1, dns1_len, "8.8.8.8");
    field_copy(dns2, dns2_len, "8.8.4.4");
    *use_static = false;

    const char *p = cmd;
    if (!strncmp(p, "wifi_config", 11)) p += 11;
    while (*p == ' ' || *p == '|') p++;

    while (*p) {
        const char *eq = strchr(p, '=');
        if (!eq) break;
        char key[16] = {0};
        size_t klen = (size_t)(eq - p);
        if (klen >= sizeof(key)) klen = sizeof(key) - 1;
        for (size_t i = 0; i < klen; ++i) key[i] = (char)tolower((unsigned char)p[i]);

        const char *value = eq + 1;
        const char *end = strchr(value, '|');
        size_t vlen = end ? (size_t)(end - value) : strlen(value);
        char tmp[96] = {0};
        if (vlen >= sizeof(tmp)) vlen = sizeof(tmp) - 1;
        memcpy(tmp, value, vlen);
        tmp[vlen] = '\0';

        if (!strcmp(key, "ssid")) field_copy(ssid, ssid_len, tmp);
        else if (!strcmp(key, "pass")) field_copy(pass, pass_len, tmp);
        else if (!strcmp(key, "host")) field_copy(host, host_len, tmp);
        else if (!strcmp(key, "static")) *use_static = (!strcmp(tmp, "1") || !strcmp(tmp, "sim") || !strcmp(tmp, "true"));
        else if (!strcmp(key, "ip")) field_copy(ip, ip_len, tmp);
        else if (!strcmp(key, "gw")) field_copy(gw, gw_len, tmp);
        else if (!strcmp(key, "mask")) field_copy(mask, mask_len, tmp);
        else if (!strcmp(key, "dns1")) field_copy(dns1, dns1_len, tmp);
        else if (!strcmp(key, "dns2")) field_copy(dns2, dns2_len, tmp);

        if (!end) break;
        p = end + 1;
        while (*p == ' ') p++;
    }
}

void jr_print_help(void){ printf("\nJrBot v4. Comandos:\n neutro feliz triste animado bravo surpreso pensando cetico sono confuso piscando amor brincalhao preocupado cool bateria\n demo status help\n wifi_config ssid=NOME|pass=SENHA|static=0|ip=192.168.0.50|gw=192.168.0.1|mask=255.255.255.0\n wifi_clear\n\n"); }

bool jr_handle_command(const char *cmd, char *response, size_t response_len) {
    char base[48];
    lower_copy(base, sizeof(base), cmd);

    if (!strcmp(base, "status")) {
        snprintf(response, response_len, "JR_STATUS v=4 version=%s expression=%s demo=%d wifi_config=%d wifi=%d ssid=%s ip=%s commands=%lu frames=%lu",
            JR_APP_VERSION, jr_face_expression_name(), jr_face_demo_enabled() ? 1 : 0,
            jr_wifi_is_configured() ? 1 : 0, jr_wifi_is_connected() ? 1 : 0, jr_wifi_ssid(), jr_wifi_ip(),
            (unsigned long)jr_face_command_count(), (unsigned long)jr_face_frame_count());
        jr_face_print_status();
        printf("%s\n", response);
        return true;
    }
    if (!strcmp(base, "help") || !strcmp(base, "ajuda")) {
        snprintf(response, response_len, "comandos: neutro feliz triste animado bravo surpreso pensando cetico sono confuso piscando amor brincalhao preocupado cool bateria demo status wifi_config wifi_clear");
        jr_print_help();
        return true;
    }
    if (!strncmp(base, "wifi_config", 11)) {
        char ssid[33] = "", pass[65] = "", host[33] = "jrbot", ip[16] = "192.168.0.50", gw[16] = "192.168.0.1", mask[16] = "255.255.255.0", dns1[16] = "8.8.8.8", dns2[16] = "8.8.4.4";
        bool use_static = false;
        parse_wifi_config(cmd, ssid, sizeof(ssid), pass, sizeof(pass), host, sizeof(host), &use_static, ip, sizeof(ip), gw, sizeof(gw), mask, sizeof(mask), dns1, sizeof(dns1), dns2, sizeof(dns2));
        bool ok = jr_wifi_configure(ssid, pass, host, use_static, ip, gw, mask, dns1, dns2, response, (unsigned)response_len);
        if (ok) jr_portal_start();
        return ok;
    }
    if (!strcmp(base, "wifi_clear")) {
        return jr_wifi_clear(response, (unsigned)response_len);
    }
    if (!strcmp(base, "demo")) {
        jr_face_set_demo(!jr_face_demo_enabled());
        snprintf(response, response_len, "JR_OK demo=%d", jr_face_demo_enabled() ? 1 : 0);
        printf("JR_OK demo=%d\n", jr_face_demo_enabled() ? 1 : 0);
        return true;
    }
    if (jr_face_set_expression(base)) {
        jr_face_increment_command_count();
        snprintf(response, response_len, "JR_OK command=%lu expression=%s", (unsigned long)jr_face_command_count(), jr_face_expression_name());
        printf("JR_OK command=%lu expression=%s\n", (unsigned long)jr_face_command_count(), jr_face_expression_name());
        return true;
    }
    snprintf(response, response_len, "JR_ERROR comando_desconhecido=%s", cmd);
    printf("JR_ERROR comando_desconhecido=%s\n", cmd);
    return false;
}

static void terminal_task(void *p){
    uint8_t rx[96]; char cmd[192]={0}; size_t n=0; char response[256]; jr_print_help();
    while(true){
        int count=uart_read_bytes(UART_NUM_0,rx,sizeof(rx),pdMS_TO_TICKS(100));
        for(int i=0;i<count;i++){
            char ch=(char)rx[i];
            if(ch=='\r'||ch=='\n'){
                if(!n)continue;
                cmd[n]='\0';
                jr_handle_command(cmd,response,sizeof(response));
                n=0;
            } else if(n<sizeof(cmd)-1) cmd[n++]=ch;
        }
    }
}

void jr_terminal_start(void) {
    esp_err_t uart_result=uart_driver_install(UART_NUM_0,2048,0,0,NULL,0);
    if(uart_result!=ESP_OK&&uart_result!=ESP_ERR_INVALID_STATE) ESP_ERROR_CHECK(uart_result);
    xTaskCreate(terminal_task,"terminal",4096,NULL,5,NULL);
}
