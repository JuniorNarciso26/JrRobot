#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "jr_config.h"
#include "jr_board.h"
#include "jr_face.h"

#define OLED_SDA JR_OLED_SDA_GPIO
#define OLED_SCL JR_OLED_SCL_GPIO
#define OLED_ADDRESS_PRIMARY 0x3C
#define OLED_ADDRESS_SECONDARY 0x3D
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_PAGES 8

static const char *TAG = "jrbot_face";
static uint8_t frame[OLED_WIDTH * OLED_PAGES];
static uint8_t oled_address = OLED_ADDRESS_PRIMARY;
static i2c_master_bus_handle_t i2c_bus;
static i2c_master_dev_handle_t oled_device;
typedef enum {
    FACE_NEUTRAL = 0,
    FACE_HAPPY,
    FACE_SAD,
    FACE_EXCITED,
    FACE_ANGRY,
    FACE_SURPRISED,
    FACE_THINKING,
    FACE_SKEPTICAL,
    FACE_SLEEPY,
    FACE_CONFUSED,
    FACE_WINKING,
    FACE_LOVE,
    FACE_PLAYFUL,
    FACE_WORRIED,
    FACE_COOL,
    FACE_BATTERY_LOW,
} face_expression_t;

static portMUX_TYPE state_lock = portMUX_INITIALIZER_UNLOCKED;
static face_expression_t current_expression = FACE_NEUTRAL;
static bool demo_mode;
static jr_face_status_t status_data = {.state=JR_OLED_UNINITIALIZED,.address=OLED_ADDRESS_PRIMARY,.last_error=ESP_ERR_INVALID_STATE};
static bool oled_ready;
static bool start_attempted;
static esp_err_t io_error;
static uint32_t retry_started_ms;
static uint32_t retry_delay_ms=500;
static bool blink_active;
static int look_dx = 0;
static int look_dy = 0;
static uint32_t next_blink_ms = 2500;
static uint32_t blink_started_ms = 0;
static uint32_t last_auto_look_ms = 0;
static uint32_t last_demo_ms = 0;

static const char *face_expression_to_name(face_expression_t expression) {
    switch (expression) {
        case FACE_HAPPY: return "happy";
        case FACE_SAD: return "sad";
        case FACE_EXCITED: return "excited";
        case FACE_ANGRY: return "angry";
        case FACE_SURPRISED: return "surprised";
        case FACE_THINKING: return "thinking";
        case FACE_SKEPTICAL: return "skeptical";
        case FACE_SLEEPY: return "sleepy";
        case FACE_CONFUSED: return "confused";
        case FACE_WINKING: return "winking";
        case FACE_LOVE: return "love";
        case FACE_PLAYFUL: return "playful";
        case FACE_WORRIED: return "worried";
        case FACE_COOL: return "cool";
        case FACE_BATTERY_LOW: return "battery_low";
        case FACE_NEUTRAL:
        default: return "neutral";
    }
}

static uint32_t now_ms(void) { return (uint32_t)(esp_timer_get_time() / 1000ULL); }

static bool oled_send(uint8_t control, const uint8_t *bytes, size_t length) {
    uint8_t transfer[129];
    if (!oled_device || !bytes || length>sizeof(transfer)-1) { io_error=ESP_ERR_INVALID_ARG; return false; }
    transfer[0]=control;
    memcpy(transfer+1,bytes,length);
    io_error=i2c_master_transmit(oled_device,transfer,length+1,25);
    return io_error==ESP_OK;
}
static bool oled_command(uint8_t command) { return oled_send(0x00,&command,1); }
static bool oled_show(void);

static bool oled_release(void) {
    /* Keep handles on cleanup errors; never allocate another live controller. */
    if (oled_device) {
        esp_err_t err=i2c_master_bus_rm_device(oled_device);
        if (err!=ESP_OK) { io_error=err; return false; }
        oled_device=NULL;
    }
    if (i2c_bus) {
        esp_err_t err=i2c_del_master_bus(i2c_bus);
        if (err!=ESP_OK) { io_error=err; return false; }
        i2c_bus=NULL;
    }
    return true;
}
static bool oled_select_address(void) {
    const uint8_t candidates[]={OLED_ADDRESS_PRIMARY,OLED_ADDRESS_SECONDARY};
    for (size_t i=0;i<sizeof(candidates);i++) {
        io_error=i2c_master_probe(i2c_bus,candidates[i],25);
        if (io_error==ESP_OK) {
            oled_address=candidates[i];
            i2c_device_config_t config={.dev_addr_length=I2C_ADDR_BIT_LEN_7,
                .device_address=oled_address,.scl_speed_hz=JR_OLED_I2C_HZ};
            io_error=i2c_master_bus_add_device(i2c_bus,&config,&oled_device);
            return io_error==ESP_OK;
        }
    }
    return false;
}
static bool oled_init(void) {
    /* SSD1306 ONLY: page addressing; clean display RAM before display-on. */
    const uint8_t init[]={0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0x00,0x40,0x8D,0x14,0x20,0x02,0xA1,0xC8,0xDA,0x12,0x81,0xCF,0xD9,0xF1,0xDB,0x40,0xA4,0xA6};
    if (!oled_select_address() || !oled_send(0x00,init,sizeof(init))) return false;
    memset(frame,0,sizeof(frame));
    return oled_show() && oled_command(0xAF);
}

static void set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    uint8_t *cell = &frame[x + (y / 8) * OLED_WIDTH];
    uint8_t mask = 1U << (y % 8);
    if (on) *cell |= mask; else *cell &= (uint8_t)~mask;
}
static void fill_rect(int x, int y, int w, int h, bool on) { for(int py=y; py<y+h; ++py) for(int px=x; px<x+w; ++px) set_pixel(px,py,on); }
static void fill_round_rect(int x, int y, int w, int h, int r, bool on) {
    for (int py=y; py<y+h; ++py) for (int px=x; px<x+w; ++px) {
        int l=px-x, rt=x+w-1-px, t=py-y, b=y+h-1-py;
        int dx=l<r?r-l:(rt<r?r-rt:0), dy=t<r?r-t:(b<r?r-b:0);
        if (dx*dx+dy*dy <= r*r) set_pixel(px,py,on);
    }
}
static void draw_line(int x0,int y0,int x1,int y1,bool on,int thick) {
    int dx=x1>x0?x1-x0:x0-x1, sx=x0<x1?1:-1, dy=-(y1>y0?y1-y0:y0-y1), sy=y0<y1?1:-1, err=dx+dy;
    while(true){ for(int yy=-thick/2; yy<-thick/2+thick; ++yy) for(int xx=-thick/2; xx<-thick/2+thick; ++xx) set_pixel(x0+xx,y0+yy,on); if(x0==x1&&y0==y1)break; int e2=2*err; if(e2>=dy){err+=dy;x0+=sx;} if(e2<=dx){err+=dx;y0+=sy;} }
}
static void draw_circle(int cx,int cy,int r,bool on,int thick){ for(int a=0;a<360;a+=3){ float rad=a*3.14159f/180.0f; for(int t=0;t<thick;t++) set_pixel(cx+(int)((r-t)*cosf(rad)), cy+(int)((r-t)*sinf(rad)), on);} }
static void draw_arc(int cx,int cy,int r,int start,int end,bool on,int thick){ for(int a=start;a<=end;a+=3){ float rad=a*3.14159f/180.0f; for(int t=0;t<thick;t++) set_pixel(cx+(int)((r-t)*cosf(rad)), cy+(int)((r-t)*sinf(rad)), on);} }
static void draw_heart(int cx,int cy){ fill_round_rect(cx-9,cy-6,9,9,5,true); fill_round_rect(cx,cy-6,9,9,5,true); draw_line(cx-10,cy, cx,cy+12,true,3); draw_line(cx+10,cy, cx,cy+12,true,3); fill_rect(cx-5,cy+2,10,7,true); }

static void eye_open(int cx,int cy,int w,int h,int pdx,int pdy){ fill_round_rect(cx-w/2,cy-h/2,w,h,7,true); fill_round_rect(cx-4+pdx,cy-7+pdy,8,14,4,false); }
static void eye_line(int cx,int cy,int w){ fill_round_rect(cx-w/2,cy-2,w,4,2,true); }
static void mouth_happy(void){ draw_arc(64,45,15,20,160,true,3); }
static void mouth_sad(void){ draw_arc(64,58,14,200,340,true,3); }
static void mouth_flat(void){ draw_line(52,50,76,50,true,2); }
static void mouth_o(void){ draw_circle(64,50,6,true,2); }
static void mouth_wavy(void){ draw_line(52,50,58,53,true,2); draw_line(58,53,64,50,true,2); draw_line(64,50,70,53,true,2); draw_line(70,53,76,50,true,2); }

static int blink_phase(uint32_t ms){
    if (!blink_active && (int32_t)(ms-next_blink_ms)>=0) { blink_started_ms=ms; blink_active=true; }
    if (!blink_active) return 0;
    uint32_t e = ms - blink_started_ms;
    if (e < 60) {
        return 1;
    }
    if (e < 130) {
        return 2;
    }
    if (e < 190) {
        return 1;
    }
    blink_active = false;
    next_blink_ms = ms + 2000 + (esp_random() % 4000);
    return 0;
}
static void draw_blink_eyes(int phase){ if(phase==2){eye_line(38,30,28); eye_line(90,30,28);} else {eye_open(38,30,24,12,0,0); eye_open(90,30,24,12,0,0);} }

static void draw_battery(uint32_t ms){
    draw_line(26,20,98,20,true,3); draw_line(26,44,98,44,true,3); draw_line(26,20,26,44,true,3); draw_line(98,20,98,44,true,3); fill_rect(101,27,6,11,true);
    if ((ms/500)%2==0) fill_rect(32,26,12,13,true);
    draw_line(45,51,83,51,true,2);
}

static void draw_face(face_expression_t f, uint32_t ms) {
    memset(frame,0,sizeof(frame));
    int pulse = (ms/250)%2;
    int phase = blink_phase(ms);
    int pdx=look_dx, pdy=look_dy;
    if (f==FACE_BATTERY_LOW){ draw_battery(ms); return; }
    if (f!=FACE_SLEEPY && f!=FACE_WINKING && f!=FACE_LOVE && f!=FACE_COOL && phase){ draw_blink_eyes(phase); mouth_flat(); return; }
    switch(f){
        case FACE_HAPPY: eye_line(38,27,26); eye_line(90,27,26); mouth_happy(); break;
        case FACE_SAD: eye_open(38,30,24,24,pdx,3); eye_open(90,30,24,24,pdx,3); draw_line(28,17,48,22,true,3); draw_line(80,22,100,17,true,3); mouth_sad(); break;
        case FACE_EXCITED: eye_open(38,29,28+pulse*4,30+pulse*2,0,-2); eye_open(90,29,28+pulse*4,30+pulse*2,0,-2); draw_line(27,12,49,10,true,3); draw_line(79,10,101,12,true,3); draw_circle(64,50,7+pulse,true,3); break;
        case FACE_ANGRY: eye_open(38,31,27,18,0,0); eye_open(90,31,27,18,0,0); draw_line(25,15,51,24,true,4); draw_line(77,24,103,15,true,4); mouth_sad(); break;
        case FACE_SURPRISED: draw_circle(38,30,15,true,3); draw_circle(90,30,15,true,3); fill_round_rect(35,27,6,6,3,true); fill_round_rect(87,27,6,6,3,true); draw_line(28,12,48,10,true,3); draw_line(80,10,100,12,true,3); mouth_o(); break;
        case FACE_THINKING: eye_open(38,30,24,26,4,-2); eye_open(90,31,24,13,4,-1); draw_line(27,14,49,11,true,3); draw_line(80,18,100,18,true,2); draw_line(56,50,72,53,true,2); break;
        case FACE_SKEPTICAL: eye_open(38,31,24,13,-4,0); eye_open(90,30,24,25,-4,0); draw_line(26,17,50,13,true,3); draw_line(80,18,100,19,true,3); mouth_flat(); break;
        case FACE_SLEEPY: eye_line(38,30,26); eye_line(90,30,26); mouth_o(); draw_line(104,12,114,12,true,2); draw_line(114,12,104,22,true,2); draw_line(104,22,114,22,true,2); break;
        case FACE_CONFUSED: eye_open(38,30,24,23,-3,0); eye_open(90,31,24,23,4,0); draw_arc(38,13,12,200,340,true,3); draw_line(78,16,102,22,true,3); mouth_wavy(); break;
        case FACE_WINKING: eye_line(38,30,26); eye_open(90,30,25,27,0,0); mouth_happy(); break;
        case FACE_LOVE: draw_heart(38,27+pulse); draw_heart(90,27+pulse); mouth_happy(); break;
        case FACE_PLAYFUL: eye_line(38,27,25); eye_line(90,27,25); mouth_happy(); fill_round_rect(60,49,8,9,4,true); break;
        case FACE_WORRIED: eye_open(38,30,24,24,0,1); eye_open(90,30,24,24,0,1); draw_line(28,16,48,20,true,3); draw_line(80,20,100,16,true,3); mouth_wavy(); break;
        case FACE_COOL: fill_round_rect(22,23,34,17,4,true); fill_round_rect(72,23,34,17,4,true); draw_line(56,30,72,30,true,3); mouth_happy(); break;
        case FACE_NEUTRAL: default: eye_open(38,30,24,28,pdx,pdy); eye_open(90,30,24,28,pdx,pdy); mouth_flat(); break;
    }
}
static bool oled_show(void){ for(uint8_t page=0; page<OLED_PAGES; ++page){ if(!oled_command(0xB0|page)||!oled_command(0x00)||!oled_command(0x10)||!oled_send(0x40,&frame[page*OLED_WIDTH],OLED_WIDTH)) return false;} return true; }

bool jr_face_set_expression(const char *c){
    if (!c) return false;
    face_expression_t next;
    if(!strcmp(c,"neutro")||!strcmp(c,"neutral")) next=FACE_NEUTRAL;
    else if(!strcmp(c,"feliz")||!strcmp(c,"happy")) next=FACE_HAPPY;
    else if(!strcmp(c,"triste")||!strcmp(c,"sad")) next=FACE_SAD;
    else if(!strcmp(c,"animado")||!strcmp(c,"excited")) next=FACE_EXCITED;
    else if(!strcmp(c,"bravo")||!strcmp(c,"angry")) next=FACE_ANGRY;
    else if(!strcmp(c,"surpreso")||!strcmp(c,"surprised")) next=FACE_SURPRISED;
    else if(!strcmp(c,"pensando")||!strcmp(c,"thinking")) next=FACE_THINKING;
    else if(!strcmp(c,"cetico")||!strcmp(c,"skeptical")) next=FACE_SKEPTICAL;
    else if(!strcmp(c,"sono")||!strcmp(c,"sleepy")) next=FACE_SLEEPY;
    else if(!strcmp(c,"confuso")||!strcmp(c,"confused")) next=FACE_CONFUSED;
    else if(!strcmp(c,"piscando")||!strcmp(c,"winking")) next=FACE_WINKING;
    else if(!strcmp(c,"amor")||!strcmp(c,"love")) next=FACE_LOVE;
    else if(!strcmp(c,"brincalhao")||!strcmp(c,"playful")) next=FACE_PLAYFUL;
    else if(!strcmp(c,"preocupado")||!strcmp(c,"worried")) next=FACE_WORRIED;
    else if(!strcmp(c,"cool")||!strcmp(c,"tranquilo")) next=FACE_COOL;
    else if(!strcmp(c,"bateria")||!strcmp(c,"battery")) next=FACE_BATTERY_LOW;
    else return false;
    portENTER_CRITICAL(&state_lock);
    current_expression=next; demo_mode=false;
    portEXIT_CRITICAL(&state_lock);
    return true;
}

const char *jr_face_state_name(jr_oled_state_t state) {
    switch(state) {
        case JR_OLED_READY:return "ready";
        case JR_OLED_DEGRADED:return "degraded";
        case JR_OLED_RECOVERING:return "recovering";
        case JR_OLED_OFFLINE:return "offline";
        default:return "uninitialized";
    }
}
void jr_face_get_status(jr_face_status_t *out) {
    if (!out) return;
    portENTER_CRITICAL(&state_lock);
    *out=status_data;
    out->demo=demo_mode;
    const char *name=face_expression_to_name(current_expression);
    memcpy(out->expression,name,strlen(name)+1);
    portEXIT_CRITICAL(&state_lock);
}
const char *jr_face_expression_name(void) {
    portENTER_CRITICAL(&state_lock);
    face_expression_t f=current_expression;
    portEXIT_CRITICAL(&state_lock);
    return face_expression_to_name(f);
}
void jr_face_set_demo(bool enabled) { portENTER_CRITICAL(&state_lock); demo_mode=enabled; portEXIT_CRITICAL(&state_lock); }
bool jr_face_demo_enabled(void) { jr_face_status_t s; jr_face_get_status(&s); return s.demo; }
uint32_t jr_face_command_count(void) { jr_face_status_t s; jr_face_get_status(&s); return s.commands; }
uint32_t jr_face_frame_count(void) { jr_face_status_t s; jr_face_get_status(&s); return s.rendered; }
void jr_face_increment_command_count(void) { portENTER_CRITICAL(&state_lock); status_data.commands++; portEXIT_CRITICAL(&state_lock); }
void jr_face_print_status(void) {
    jr_face_status_t s; jr_face_get_status(&s);
    printf("JR_FACE oled=%s expression=%s sda=1 scl=2 rendered=%lu tx_ok=%lu tx_fail=%lu skipped=%lu init_fail=%lu last_error=%s\n",
        jr_face_state_name(s.state),s.expression,(unsigned long)s.rendered,(unsigned long)s.tx_ok,
        (unsigned long)s.tx_failed,(unsigned long)s.skipped,(unsigned long)s.init_failed,esp_err_to_name(s.last_error));
}

esp_err_t jr_face_start(void) {
    if (oled_ready) return ESP_OK;
    bool recovery=start_attempted;
    start_attempted=true;
    portENTER_CRITICAL(&state_lock);
    status_data.state=JR_OLED_RECOVERING;
    portEXIT_CRITICAL(&state_lock);
    io_error=ESP_OK;
    bool ok=oled_release();
    if (ok) {
        i2c_master_bus_config_t config={.i2c_port=JR_OLED_I2C_PORT,.sda_io_num=OLED_SDA,.scl_io_num=OLED_SCL,
            .clk_source=I2C_CLK_SRC_DEFAULT,.glitch_ignore_cnt=7,.flags.enable_internal_pullup=true};
        io_error=i2c_new_master_bus(&config,&i2c_bus);
        ok=io_error==ESP_OK && oled_init();
    }
    esp_err_t failure=io_error==ESP_OK?ESP_FAIL:io_error;
    if (!ok) oled_release();
    oled_ready=ok;
    retry_started_ms=now_ms();
    portENTER_CRITICAL(&state_lock);
    status_data.address=oled_address;
    status_data.state=ok?JR_OLED_READY:JR_OLED_OFFLINE;
    status_data.last_error=ok?ESP_OK:failure;
    if (ok) {
        status_data.consecutive_failures=0;
        if (recovery) status_data.recoveries++;
    } else status_data.init_failed++;
    portEXIT_CRITICAL(&state_lock);
    if (ok) retry_delay_ms=500;
    else if (recovery && retry_delay_ms<8000) retry_delay_ms*=2;
    if (!ok) ESP_LOGW(TAG,"OLED offline: %s; retry=%lu ms",esp_err_to_name(failure),(unsigned long)retry_delay_ms);
    return ok?ESP_OK:failure;
}

void jr_face_step(uint32_t ms) {
    static unsigned demo_i;
    if (!oled_ready && (uint32_t)(ms-retry_started_ms)>=retry_delay_ms) jr_face_start();
    portENTER_CRITICAL(&state_lock);
    bool demo=demo_mode;
    if (demo && (uint32_t)(ms-last_demo_ms)>2500) {
        last_demo_ms=ms; current_expression=(face_expression_t)(demo_i++%16);
    }
    face_expression_t expression=current_expression;
    portEXIT_CRITICAL(&state_lock);
    if (!demo && (uint32_t)(ms-last_auto_look_ms)>1800) {
        last_auto_look_ms=ms; look_dx=(int)(esp_random()%9)-4; look_dy=(int)(esp_random()%5)-2;
    }
    draw_face(expression,ms);
    bool attempted=oled_ready;
    bool sent=attempted && oled_show();
    portENTER_CRITICAL(&state_lock);
    status_data.rendered++;
    if (!attempted) status_data.skipped++;
    else if (sent) {
        status_data.tx_ok++; status_data.last_success_ms=ms;
        status_data.last_error=ESP_OK; status_data.consecutive_failures=0; status_data.state=JR_OLED_READY;
    } else {
        status_data.tx_failed++; status_data.consecutive_failures++;
        status_data.last_error=io_error; status_data.state=JR_OLED_DEGRADED;
    }
    portEXIT_CRITICAL(&state_lock);
    if (attempted && !sent) {
        oled_ready=false; retry_started_ms=ms; retry_delay_ms=500;
    }
}
void jr_face_loop(void) {
    uint32_t last_log=now_ms();
    for (;;) {
        uint32_t ms=now_ms(); jr_face_step(ms);
        if ((uint32_t)(ms-last_log)>=4000) { last_log=ms; jr_face_print_status(); }
        vTaskDelay(pdMS_TO_TICKS(80));
    }
}
