#include <assert.h>
#include <stdarg.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "jr_board.h"
#include "jr_commands.h"
#include "jr_face.h"
#include "jr_wifi.h"
#include "freertos/task.h"
void app_main(void);
static jmp_buf done;
static int terminal,display,loop,wifi,portal,sequence,term_order,face_order;
static bool network_ready;
static esp_err_t command_result;
static char logs[4096];
void diag_test_log(const char *tag,const char *fmt,...) {(void)tag;size_t n=strlen(logs);va_list ap;va_start(ap,fmt);vsnprintf(logs+n,sizeof(logs)-n,fmt,ap);va_end(ap);}
void jr_wifi_prepare(void) {}
esp_err_t jr_commands_init(void) {return command_result;}
void jr_terminal_start(void) {terminal++;term_order=++sequence;}
esp_err_t jr_face_start(void) {display++;face_order=++sequence;return ESP_FAIL;}
void jr_face_loop(void) {loop++;longjmp(done,1);}
void jr_wifi_start(void) {wifi++;}
bool jr_wifi_network_ready(void) {return network_ready;}
void jr_portal_start(void) {portal++;}
void vTaskDelay(TickType_t n) {(void)n;longjmp(done,1);}
int main(int argc,char **argv) {
 assert(argc==2);network_ready=!strcmp(argv[1],"network");command_result=!strcmp(argv[1],"mutex_error")?ESP_FAIL:ESP_OK;
 if (!setjmp(done)) {app_main();assert(!"app_main returned");}
 assert(terminal==1 && wifi==1 && portal==(network_ready?1:0));
 assert(display==1 && loop==1 && term_order<face_order);
 assert(strstr(logs,"JRBotV2_2026-09-06-07:10"));assert(strstr(logs,"terminal remains available"));
 puts("PASS final boot control flow (mock APIs; no hardware)");return 0;
}
