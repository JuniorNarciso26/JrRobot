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
#include "jr_face.h"

void jr_print_help(void){ printf("\nJrBot v3. Comandos:\n neutro feliz triste animado bravo surpreso pensando cetico sono confuso piscando amor brincalhao preocupado cool bateria\n demo status help\n Wi-Fi: abrir http://IP_DO_ESP32/ no navegador\n\n"); }
bool jr_handle_command(const char *cmd, char *response, size_t response_len) {
    if (!strcmp(cmd, "status")) {
        snprintf(response, response_len, "JR_STATUS v=3 version=%s expression=%s demo=%d wifi=%d ip=%s commands=%lu frames=%lu",
            JR_APP_VERSION, jr_face_expression_name(), jr_face_demo_enabled() ? 1 : 0,
            jr_wifi_is_connected() ? 1 : 0, jr_wifi_ip(),
            (unsigned long)jr_face_command_count(), (unsigned long)jr_face_frame_count());
        jr_face_print_status();
        return true;
    }
    if (!strcmp(cmd, "help") || !strcmp(cmd, "ajuda")) {
        snprintf(response, response_len, "comandos: neutro feliz triste animado bravo surpreso pensando cetico sono confuso piscando amor brincalhao preocupado cool bateria demo status");
        jr_print_help();
        return true;
    }
    if (!strcmp(cmd, "demo")) {
        jr_face_set_demo(!jr_face_demo_enabled());
        snprintf(response, response_len, "JR_OK demo=%d", jr_face_demo_enabled() ? 1 : 0);
        printf("JR_OK demo=%d\n", jr_face_demo_enabled() ? 1 : 0);
        return true;
    }
    if (jr_face_set_expression(cmd)) {
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
    uint8_t rx[64]; char cmd[32]={0}; size_t n=0; char response[192]; jr_print_help();
    while(true){ int count=uart_read_bytes(UART_NUM_0,rx,sizeof(rx),pdMS_TO_TICKS(100)); for(int i=0;i<count;i++){ char ch=(char)rx[i]; if(ch=='\r'||ch=='\n'){ if(!n)continue; cmd[n]='\0'; jr_handle_command(cmd,response,sizeof(response)); n=0; } else if(n<sizeof(cmd)-1) cmd[n++]=(char)tolower((unsigned char)ch); }}
}




void jr_terminal_start(void) {
    esp_err_t uart_result=uart_driver_install(UART_NUM_0,2048,0,0,NULL,0);
    if(uart_result!=ESP_OK&&uart_result!=ESP_ERR_INVALID_STATE) ESP_ERROR_CHECK(uart_result);
    xTaskCreate(terminal_task,"terminal",4096,NULL,5,NULL);
}
