#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "jr_board.h"
#include "jr_camera_diag.h"
#include "esp_camera.h"
#include "esp_psram.h"
static bool psram=true;
static int init_calls,deinit_calls,get_calls,returned;
static const char *scenario;
static uint8_t jpeg[]={0xff,0xd8,0xff,0xd9};
static sensor_t sensor={.id={.PID=0x5640}};
static camera_fb_t frame={.buf=jpeg,.len=4,.width=320,.height=240,.format=PIXFORMAT_JPEG};
const char *esp_err_to_name(esp_err_t e) {return e==ESP_OK?"ESP_OK":"ESP_FAIL";}
bool esp_psram_is_initialized(void) {return psram;}
esp_err_t esp_camera_init(const camera_config_t *c) {
 init_calls++;
 assert(c->pin_xclk==15 && c->pin_sccb_sda==4 && c->pin_sccb_scl==5);
 assert(c->pin_d0==11 && c->pin_d1==9 && c->pin_d2==8 && c->pin_d3==10);
 assert(c->pin_d4==12 && c->pin_d5==18 && c->pin_d6==17 && c->pin_d7==16);
 assert(c->pin_vsync==6 && c->pin_href==7 && c->pin_pclk==13);
 assert(c->frame_size==FRAMESIZE_QVGA && c->fb_count==1 && c->pixel_format==PIXFORMAT_JPEG);
 return !strcmp(scenario,"init_error")?ESP_FAIL:ESP_OK;
}
sensor_t *esp_camera_sensor_get(void) {return !strcmp(scenario,"sensor_missing")?NULL:&sensor;}
camera_fb_t *esp_camera_fb_get(void) {get_calls++;return !strcmp(scenario,"no_frame")?NULL:&frame;}
void esp_camera_fb_return(camera_fb_t *p) {assert(p==&frame);returned++;}
esp_err_t esp_camera_deinit(void) {deinit_calls++;return !strcmp(scenario,"deinit_error")?ESP_FAIL:ESP_OK;}
int main(int argc,char **argv) {
 assert(argc==2);scenario=argv[1];char out[768]={0};
 if (!strcmp(scenario,"psram"))psram=false;
 if (!strcmp(scenario,"corrupt"))jpeg[0]=0;
 if (!strcmp(scenario,"null_buffer"))frame.buf=NULL;
 if (!strcmp(scenario,"wrong_format"))frame.format=0;
 if (!strcmp(scenario,"zero_dimensions"))frame.height=0;
 if (!strcmp(scenario,"small_buffer")) {
  assert(!jr_camera_test_once(out,16));assert(init_calls==0);return 0;
 }
 bool ok=jr_camera_test_once(out,sizeof(out));
 if (!JR_CAMERA_ENABLED) {
  assert(!ok && strstr(out,"pins_not_confirmed") && init_calls==0);
 } else if (!strcmp(scenario,"success") || !strcmp(scenario,"repeat")) {
  assert(ok && strstr(out,"frame_received") && strstr(out,"optical_check=pending"));
  assert(init_calls==1 && deinit_calls==1 && returned==1);
  if (!strcmp(scenario,"repeat")) {assert(jr_camera_test_once(out,sizeof(out)));assert(init_calls==2 && returned==2 && deinit_calls==2);}
 } else if (!strcmp(scenario,"psram")) {
  assert(!ok && init_calls==0 && strstr(out,"psram_unavailable"));
 } else if (!strcmp(scenario,"init_error")) {
  assert(!ok && init_calls==1 && get_calls==0 && deinit_calls==0);
 } else {
  assert(!ok && deinit_calls==1);
  if (!strcmp(scenario,"sensor_missing"))assert(get_calls==0 && returned==0);
  else if (!strcmp(scenario,"no_frame"))assert(returned==0);
  else assert(returned==1);
  if (!strcmp(scenario,"deinit_error")) {
   assert(strstr(out,"restart_required=1"));
   assert(!jr_camera_test_once(out,sizeof(out)));assert(init_calls==1 && deinit_calls==1);
  }
 }
 puts("PASS camera flow (mock sensor, not a real image)");return 0;
}
