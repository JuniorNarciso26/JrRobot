"""Integration resource budgets: actual flash files and actual SIMD pool C code."""
from pathlib import Path
import json
import shutil
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
import check_voice_unified as guard

class UnifiedTest(unittest.TestCase):
    def test_flash_budget_real_babytalk_and_oversize_missing_images(self):
        model=ROOT.parent/'babytalk/models/citrinet256_int4.mmrt'
        if not model.exists():self.skipTest('official BabyTalk model required')
        with tempfile.TemporaryDirectory() as tmp:
            folder=Path(tmp)
            (folder/'app.bin').write_bytes(b'a'*1024)
            (folder/'wake.bin').write_bytes(b'w'*819360)
            meta={'flash_files':{'0x10000':'app.bin','0x810000':'wake.bin','0xA00000':str(model)}}
            (folder/'flasher_args.json').write_text(json.dumps(meta))
            budget=guard.check(folder)
            self.assertEqual(budget['stt_model']['bytes'],5991232)
            with (folder/'app.bin').open('wb') as f:f.truncate(0x800001)
            with self.assertRaises(RuntimeError):guard.check(folder)
            (folder/'app.bin').write_bytes(b'a')
            meta['flash_files'].pop('0x810000')
            (folder/'flasher_args.json').write_text(json.dumps(meta))
            with self.assertRaises(RuntimeError):guard.check(folder)

    @unittest.skipUnless(shutil.which('gcc'),'host compiler required')
    def test_real_simd_pool_is_deferred_exact_size_and_returns_memory(self):
        source=ROOT.parent/'babytalk'
        if not source.exists():self.skipTest('upstream headers required')
        with tempfile.TemporaryDirectory() as tmp:
            folder=Path(tmp)
            (folder/'esp_heap_caps.h').write_text('#include <stddef.h>\n#define MALLOC_CAP_INTERNAL 1\n#define MALLOC_CAP_8BIT 2\nvoid *heap_caps_aligned_alloc(size_t,size_t,int);\nvoid heap_caps_free(void*);\n')
            (folder/'test.c').write_text(r'''
#include <assert.h>
#include <stdlib.h>
#include "sram_pool.h"
#include "mmrt_s3.h"
static int allocated,freed,deny;
void *heap_caps_aligned_alloc(size_t align,size_t size,int caps){
 assert(align==16 && size==73728 && caps==3);++allocated;return deny?NULL:malloc(size);
}
void heap_caps_free(void *p){assert(p);++freed;free(p);}
int main(void){
 assert(!sram_pool_reserve() && allocated==0);
 void *first=sram_pool_acquire(1,NULL);assert(first && allocated==1);
 assert(sram_pool_acquire(1,NULL)==first && allocated==1);
 sram_pool_release(2);assert(freed==0);
 sram_pool_release(1);assert(freed==1);
 sram_pool_release(1);assert(freed==1);
 assert(sram_pool_acquire(1,NULL) && allocated==2);
 sram_pool_release(1);assert(freed==2);
 deny=1;assert(!sram_pool_acquire(1,NULL));
}
''')
            r=subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=undefined',
                '-I',str(folder),'-I',str(source/'mmrt'),'-I',str(source/'components/sram_pool'),
                str(ROOT/'firmware/babytalk_components/sram_pool/jr_stt_pool.c'),str(folder/'test.c'),
                '-o',str(folder/'test')],capture_output=True,text=True)
            self.assertEqual(r.returncode,0,r.stderr)
            r=subprocess.run([str(folder/'test')],capture_output=True,text=True)
            self.assertEqual(r.returncode,0,r.stderr)

    @unittest.skipUnless(shutil.which('gcc'),'host compiler required')
    def test_real_wake_reply_cycle_orders_engine_and_camera_ownership(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder=Path(tmp)
            shim=r'''
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_TIMEOUT 2
#define ESP_ERR_INVALID_STATE 3
#define ESP_ERR_NO_MEM 4
#define ESP_ERR_NOT_FOUND 5
#define ESP_ERR_NOT_SUPPORTED 6
#define JRBOT_WITH_BABYTALK 1
#define JRBOT_WITH_VOICE_UNIFIED 1
#define JR_AUDIO_DIN_GPIO 42
#define JR_MIC_SCK_GPIO 21
#define JR_MIC_WS_GPIO 47
#define JR_MIC_SD_GPIO 41
#define GPIO_MODE_OUTPUT 1
#define GPIO_MODE_INPUT 2
#define GPIO_FLOATING 0
#define GPIO_PULLDOWN_ONLY 1
typedef int gpio_num_t;
static inline int gpio_set_direction(int a,int b){(void)a;(void)b;return 0;}
static inline int gpio_set_level(int a,int b){(void)a;(void)b;return 0;}
static inline int gpio_reset_pin(int a){(void)a;return 0;}
static inline int gpio_set_pull_mode(int a,int b){(void)a;(void)b;return 0;}
#define portMUX_INITIALIZER_UNLOCKED 0
typedef int portMUX_TYPE;
#define portENTER_CRITICAL(p) ((void)p)
#define portEXIT_CRITICAL(p) ((void)p)
#define pdMS_TO_TICKS(v) (v)
#define pdPASS 1
typedef void *TaskHandle_t;
void vTaskDelay(int);
void vTaskDelete(void*);
int xTaskCreatePinnedToCore(void(*)(void*),const char*,int,void*,int,void*,int);
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGW(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
const char *esp_err_to_name(int);
#define MALLOC_CAP_INTERNAL 1
size_t heap_caps_get_free_size(int);
size_t heap_caps_get_largest_free_block(int);
typedef void *i2s_chan_handle_t;
typedef struct {int id,role;} i2s_chan_config_t;
typedef struct {int rate;} clk_t;
typedef struct {int width,mode,slot_mask;} slot_t;
typedef struct {int mclk,bclk,ws,dout,din;struct{bool mclk_inv,bclk_inv,ws_inv;}invert_flags;} pins_t;
typedef struct {clk_t clk_cfg;slot_t slot_cfg;pins_t gpio_cfg;} i2s_std_config_t;
#define I2S_CHANNEL_DEFAULT_CONFIG(a,b) {a,b}
#define I2S_STD_CLK_DEFAULT_CONFIG(a) {a}
#define I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(a,b) {a,b,0}
#define I2S_NUM_AUTO -1
#define I2S_ROLE_MASTER 1
#define I2S_DATA_BIT_WIDTH_32BIT 32
#define I2S_SLOT_MODE_MONO 1
#define I2S_STD_SLOT_LEFT 1
#define I2S_GPIO_UNUSED -1
int i2s_new_channel(const i2s_chan_config_t*,void*,i2s_chan_handle_t*);
int i2s_channel_disable(i2s_chan_handle_t);
int i2s_del_channel(i2s_chan_handle_t);
int i2s_channel_enable(i2s_chan_handle_t);
int i2s_channel_init_std_mode(i2s_chan_handle_t,const i2s_std_config_t*);
int i2s_channel_read(i2s_chan_handle_t,void*,size_t,size_t*,int);
typedef void model_iface_data_t;
typedef int wakenet_state_t;
#define WAKENET_DETECTED 1
#define DET_MODE_90 90
#define ESP_WN_PREFIX "wn"
typedef struct {
 model_iface_data_t *(*create)(const char*,int);
 void(*destroy)(model_iface_data_t*);
 int(*get_samp_chunksize)(model_iface_data_t*);
 int(*get_samp_rate)(model_iface_data_t*);
 void(*clean)(model_iface_data_t*);
 wakenet_state_t(*detect)(model_iface_data_t*,int16_t*);
} esp_wn_iface_t;
typedef void srmodel_list_t;
char *esp_srmodel_filter(srmodel_list_t*,const char*,const char*);
const esp_wn_iface_t *esp_wn_handle_from_name(const char*);
srmodel_list_t *esp_srmodel_init(const char*);
void esp_srmodel_deinit(srmodel_list_t*);
'''
            (folder/'shim.h').write_text(shim)
            for name in ['esp_err.h','esp_log.h','esp_heap_caps.h','esp_wn_iface.h','esp_wn_models.h','model_path.h',
                         'jr_board.h','driver/gpio.h','driver/i2s_std.h','freertos/FreeRTOS.h','freertos/task.h']:
                p=folder/name;p.parent.mkdir(exist_ok=True);p.write_text('#include "shim.h"\n')
            harness='#include "'+str(ROOT/'firmware/module_brain/jr_voice.c')+'"\n'+r'''
#include <assert.h>
static char trace[128];static int pos,fail_capture;static bool camera_reserved;
static void mark(char ch){trace[pos++]=ch;trace[pos]=0;}
bool jr_audio_bus_acquire(uint32_t ms){(void)ms;mark('A');return true;}
void jr_audio_bus_release(void){mark('R');}
void jr_audio_stop(void){}
int jr_audio_test_tone(int hz,int ms){assert(hz==880 && ms==180 && !rx_chan);mark('B');return ESP_OK;}
int jr_camera_voice_begin(void){assert(!wn_data);camera_reserved=true;mark('K');return ESP_OK;}
void jr_camera_voice_end(void){camera_reserved=false;mark('E');}
int jr_tts_reply_to_wake(const char *text){assert(text && !wn_data && !rx_chan && camera_reserved);mark('T');return ESP_OK;}
int jr_stt_capture(i2s_chan_handle_t rx,const volatile bool *cancel){assert(rx && !wn_data && !*cancel && camera_reserved);mark('C');return fail_capture?ESP_FAIL:ESP_OK;}
void jr_stt_transcribe(const volatile bool *cancel){assert(!rx_chan && !wn_data && !*cancel);mark('S');}
void jr_stt_close(void){}
static void on_wake(const char*a,const char*b,int c){(void)a;(void)b;(void)c;mark('L');}
static void destroy(model_iface_data_t *data){assert(data);mark('D');}
static model_iface_data_t *create(const char *n,int mode){assert(!strcmp(n,"wn10_pt_olaesp") && mode==90);mark('W');return (void*)1;}
static int chunks(model_iface_data_t *d){(void)d;return 512;}
static int rate(model_iface_data_t *d){(void)d;return 16000;}
static void clean(model_iface_data_t *d){assert(d);}
static const esp_wn_iface_t iface={.create=create,.destroy=destroy,.get_samp_chunksize=chunks,.get_samp_rate=rate,.clean=clean};
char *esp_srmodel_filter(srmodel_list_t*m,const char*a,const char*b){(void)m;(void)a;(void)b;return "wn10_pt_olaesp";}
const esp_wn_iface_t *esp_wn_handle_from_name(const char *n){(void)n;return &iface;}
void vTaskDelay(int ms){(void)ms;}
int i2s_new_channel(const i2s_chan_config_t*c,void*t,i2s_chan_handle_t*r){(void)c;(void)t;*r=(void*)2;return 0;}
int i2s_channel_disable(i2s_chan_handle_t r){(void)r;return 0;}
int i2s_del_channel(i2s_chan_handle_t r){(void)r;return 0;}
int i2s_channel_enable(i2s_chan_handle_t r){(void)r;return 0;}
int i2s_channel_init_std_mode(i2s_chan_handle_t r,const i2s_std_config_t*c){(void)r;assert(c->clk_cfg.rate==16000);return 0;}
void jr_usb_terminal_emit(const char *s){(void)s;}
const char *esp_err_to_name(int err){(void)err;return "stub";}
int main(void){
 wakenet=&iface;wn_data=(void*)1;rx_chan=(void*)2;bus_owned=true;
 wake_callback=on_wake;voice_status.sample_rate=16000;voice_status.chunk_samples=512;
 assert(reply_and_resume()==ESP_OK);
 assert(!strcmp(trace,"RBDKLTACRSWAE"));
 assert(wn_data && rx_chan && bus_owned && !camera_reserved);
 pos=0;trace[0]=0;fail_capture=1;
 assert(reply_and_resume()==ESP_OK);
 assert(!strcmp(trace,"RBDKLTACRWAE"));
 assert(wn_data && rx_chan && !camera_reserved);
}
'''
            (folder/'test.c').write_text(harness)
            r=subprocess.run(['gcc','-std=c11','-ffunction-sections','-fdata-sections','-fsanitize=undefined',
                '-I',str(folder),'-I',str(ROOT/'firmware/module_brain'),'-I',str(ROOT/'firmware/module_audio'),
                '-I',str(ROOT/'firmware/module_tts'),'-I',str(ROOT/'firmware/module_camera'),'-I',str(ROOT/'firmware/core'),
                str(folder/'test.c'),'-Wl,--gc-sections','-o',str(folder/'test')],capture_output=True,text=True)
            self.assertEqual(r.returncode,0,r.stderr)
            r=subprocess.run([str(folder/'test')],capture_output=True,text=True)
            self.assertEqual(r.returncode,0,r.stderr)

if __name__=='__main__':unittest.main()
