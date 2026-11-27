"""Real upstream preparation and compiled HW04 capture checks; no ESP-IDF/board required."""
import importlib.util
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('bt_prepare', ROOT/'tools/prepare_babytalk_hw04.py')
bt = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bt)
SOURCE = ROOT.parent/'babytalk'


@unittest.skipUnless((SOURCE/'.git').exists(), 'local official BabyTalk clone required')
class BabyTalkTest(unittest.TestCase):
    def test_real_pinned_sources_model_and_repeat_prepare(self):
        with tempfile.TemporaryDirectory(prefix='jr bt ') as tmp:
            project=Path(tmp)/'project'
            bt.prepare(SOURCE,project)
            self.assertNotIn('@UPSTREAM@',(project/'CMakeLists.txt').read_text())
            for file in ('build/cache','sdkconfig','dependencies.lock'):
                path=project/file;path.parent.mkdir(exist_ok=True);path.write_text('keep')
            bt.prepare(SOURCE,project)
            for file in ('build/cache','sdkconfig','dependencies.lock'):
                self.assertEqual((project/file).read_text(),'keep')
            self.assertEqual(bt.git(SOURCE,'status','--porcelain'),'')

    def test_dirty_sources_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            clone=Path(tmp)/'clone'
            subprocess.run(['git','clone','--shared',str(SOURCE),str(clone)],check=True,capture_output=True)
            bt.git(clone,'remote','set-url','origin',bt.REPO)
            (clone/'README.md').write_text('local modification')
            with self.assertRaises(RuntimeError):bt.prepare(clone,Path(tmp)/'project')
            self.assertFalse((Path(tmp)/'project').exists())

    @unittest.skipUnless(shutil.which('gcc'),'C compiler required')
    def test_actual_capture_pause_enter_timeout_gain_and_error_cleanup(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder=Path(tmp)
            shim=r'''
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define GPIO_NUM_21 21
#define GPIO_NUM_47 47
#define GPIO_NUM_41 41
#define GPIO_NUM_42 42
#define GPIO_MODE_OUTPUT 1
#define GPIO_PULLDOWN_ONLY 1
#define I2S_GPIO_UNUSED -1
#define I2S_NUM_AUTO -1
#define I2S_ROLE_MASTER 1
#define I2S_DATA_BIT_WIDTH_32BIT 32
#define I2S_SLOT_MODE_MONO 1
#define I2S_STD_SLOT_LEFT 1
#define MALLOC_CAP_INTERNAL 1
#define MALLOC_CAP_SPIRAM 2
#define MALLOC_CAP_8BIT 4
#define pdMS_TO_TICKS(x) (x)
#define pdPASS 1
#define ESP_LOGI(tag, ...) ((void)(tag))
#define ESP_LOGE(tag, ...) ((void)(tag))
typedef void* i2s_chan_handle_t;
typedef struct {int id,role;} i2s_chan_config_t;
typedef struct {int sample_rate;} clk_t;
typedef struct {int bits,mode,slot_mask;} slot_t;
typedef struct {int mclk,bclk,ws,dout,din;} gpio_t;
typedef struct {clk_t clk_cfg;slot_t slot_cfg;gpio_t gpio_cfg;} i2s_std_config_t;
#define I2S_CHANNEL_DEFAULT_CONFIG(i,r) {i,r}
#define I2S_STD_CLK_DEFAULT_CONFIG(r) {r}
#define I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(b,m) {b,m,0}
int i2s_new_channel(const i2s_chan_config_t*,void*,i2s_chan_handle_t*);
int i2s_channel_init_std_mode(i2s_chan_handle_t,const i2s_std_config_t*);
int i2s_channel_enable(i2s_chan_handle_t);
int i2s_channel_disable(i2s_chan_handle_t);
int i2s_del_channel(i2s_chan_handle_t);
int i2s_channel_read(i2s_chan_handle_t,void*,size_t,size_t*,int);
int64_t esp_timer_get_time(void);
size_t heap_caps_get_free_size(int);
size_t heap_caps_get_largest_free_block(int);
size_t heap_caps_get_total_size(int);
void* heap_caps_malloc(size_t,int);
void vTaskDelay(int);
void vTaskDelete(void*);
int xTaskCreatePinnedToCore(void(*)(void*),const char*,int,void*,int,void*,int);
int gpio_reset_pin(int);
int gpio_set_direction(int,int);
int gpio_set_level(int,int);
int gpio_set_pull_mode(int,int);
'''
            (folder/'shim.h').write_text(shim)
            for name in ('driver/gpio.h','driver/i2s_std.h','esp_heap_caps.h','esp_log.h','esp_timer.h',
                         'freertos/FreeRTOS.h','freertos/task.h'):
                p=folder/name;p.parent.mkdir(exist_ok=True);p.write_text('#include "shim.h"\n')
            app=(ROOT/'experiments/babytalk-hw04/main/main.c').as_posix()
            harness='int fake_getchar(void);\n#define getchar fake_getchar\n#include "'+app+'"\n'+r'''
#include <assert.h>
static int reads,disabled,deleted,fail_at,scenario;
int fake_getchar(void){return scenario==3 && reads==40 ? '\n' : EOF;}
int i2s_new_channel(const i2s_chan_config_t*c,void*t,i2s_chan_handle_t*r){(void)c;(void)t;*r=(void*)1;return 0;}
int i2s_channel_init_std_mode(i2s_chan_handle_t r,const i2s_std_config_t*c){(void)r;assert(c->gpio_cfg.bclk==21 && c->gpio_cfg.ws==47 && c->gpio_cfg.din==41);assert(c->clk_cfg.sample_rate==16000);return 0;}
int i2s_channel_enable(i2s_chan_handle_t r){(void)r;return 0;}
int i2s_channel_disable(i2s_chan_handle_t r){(void)r;++disabled;return 0;}
int i2s_del_channel(i2s_chan_handle_t r){(void)r;++deleted;return 0;}
int i2s_channel_read(i2s_chan_handle_t r,void*dst,size_t n,size_t*b,int ms){
 (void)r;(void)ms;++reads;if(reads==fail_at)return -1;
 int32_t*p=dst;
 for(size_t j=0;j<n/4;++j){
   int amplitude=((int)(j%4)-2)*10000;
   if(scenario==2 || (scenario==1 && reads>28) || (scenario==4 && (reads<=48 || reads>68)))amplitude=0;
   p[j]=amplitude*65536;
 }
 *b=n;return 0;
}
int64_t esp_timer_get_time(void){return reads*16000LL;}
int main(void){
 int16_t pcm[SAMPLES];int n=-1;
 assert(capture(pcm,&n)==0 && n==SAMPLES);
 for(int i=0;i<n;++i)assert(pcm[i]==(i%4-2)*10000);
 assert(reads==633 && disabled==1 && deleted==1);
 reads=disabled=deleted=0;scenario=1;
 assert(capture(pcm,&n)==0 && n==(20+50)*256);
 assert(reads==78 && disabled==1 && deleted==1);
 reads=disabled=deleted=0;scenario=2;
 assert(capture(pcm,&n)==0 && n==0);
 assert(reads==321 && disabled==1 && deleted==1);
 reads=disabled=deleted=0;scenario=3;
 assert(capture(pcm,&n)==0 && n==32*256);
 assert(reads==40 && disabled==1 && deleted==1);
 reads=disabled=deleted=0;scenario=4;
 assert(capture(pcm,&n)==0 && n==20608);
 assert(pcm[0]==0 && pcm[n-1]==0 && reads==118);
 bool found=false;for(int i=0;i<n;++i)if(pcm[i])found=true;assert(found);
 reads=disabled=deleted=0;scenario=0;speech_threshold=30000;
 assert(capture(pcm,&n)==0 && n==0 && reads==321);
 speech_threshold=300;
 reads=disabled=deleted=0;scenario=0;mic_gain=4;
 assert(capture(pcm,&n)==0 && n==SAMPLES);
 for(int i=0;i<n;++i){int v=(i%4-2)*40000;if(v< -32768)v=-32768;if(v>32767)v=32767;assert(pcm[i]==v);}
 reads=disabled=deleted=0;fail_at=12;
 assert(capture(pcm,&n)!=0 && n==0);assert(disabled==1 && deleted==1);
}
'''
            (folder/'test.c').write_text(harness)
            subprocess.run(['gcc','-std=c11','-ffunction-sections','-fdata-sections','-fsanitize=undefined',
                            '-I',str(folder),'-I',str(SOURCE/'components/stt_engine'),'-I',str(SOURCE/'components/sram_pool'),
                            str(folder/'test.c'),'-Wl,--gc-sections','-o',str(folder/'test')],check=True,capture_output=True)
            subprocess.run([str(folder/'test')],check=True,capture_output=True,timeout=10)


if __name__=='__main__':unittest.main()
