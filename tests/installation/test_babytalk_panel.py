"""Exercise pinned adapter, flash layout and real panel capture/inference C code."""
from pathlib import Path
import importlib.util
import shutil
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
import prepare_babytalk_panel as prep
SOURCE=ROOT.parent/'babytalk'

class PanelTest(unittest.TestCase):
    @unittest.skipUnless((SOURCE/'.git').exists(),'official local clone required')
    def test_adapter_keeps_wake_partition_and_source_and_sdk_settings(self):
        with tempfile.TemporaryDirectory() as tmp:
            dest=Path(tmp)
            prep.prepare(SOURCE,dest)
            upstream=(SOURCE/'components/stt_engine/stt_engine.c').read_text()
            self.assertEqual((dest/'stt_engine.c').read_text().replace('ESP_PARTITION_SUBTYPE_ANY, "stt_model"','ESP_PARTITION_SUBTYPE_ANY, "model"'),upstream)
            config=dest/'sdkconfig'
            prep.configure(config)
            config.write_text(config.read_text()+'CONFIG_TEST_PRESERVED=y\n')
            prep.configure(config)
            text=config.read_text()
            self.assertIn('CONFIG_TEST_PRESERVED=y',text)
            self.assertEqual(text.count('CONFIG_PARTITION_TABLE_CUSTOM_FILENAME='),1)
            self.assertIn('"partitions_babytalk.csv"',text)
        previous_end=0
        parts={}
        for line in (ROOT/'firmware/partitions_babytalk.csv').read_text().splitlines():
            if line.startswith('#') or not line.strip():continue
            name,_,_,offset,size=[x.strip() for x in line.split(',')]
            start=int(offset,0);length=int(size[:-1])*1024*1024 if size.endswith('M') else int(size,0)
            self.assertGreaterEqual(start,previous_end)
            previous_end=start+length
            self.assertLessEqual(previous_end,16*1024*1024)
            parts[name]=(start,length)
        self.assertLessEqual((SOURCE/'models/citrinet256_int4.mmrt').stat().st_size,parts['stt_model'][1])
        self.assertNotEqual(parts['model'][0],parts['stt_model'][0])

    @unittest.skipUnless(shutil.which('cmake') and (ROOT.parent/'esp-idf-cmake/tools/cmake/build.cmake').exists(),
                         'official ESP-IDF CMake checkout required')
    def test_official_idf_early_dependency_scan_receives_feature_flag(self):
        idf=ROOT.parent/'esp-idf-cmake'
        for enabled in ('ON','OFF'):
            with self.subTest(enabled=enabled), tempfile.TemporaryDirectory() as tmp:
                base=Path(tmp)
                firmware=base/'firmware'
                (firmware/'vendor/babytalk').mkdir(parents=True)
                (firmware/'vendor/babytalk/paths.cmake').write_text('set(BABYTALK_ROOT "unused")\n')
                (firmware/'version.txt').write_text('regression\n')
                shutil.copy(ROOT/'firmware/CMakeLists.txt',firmware/'CMakeLists.txt')
                fake_idf=base/'idf'
                (fake_idf/'tools/cmake').mkdir(parents=True)
                (fake_idf/'tools/cmake/project.cmake').write_text(
                    f'include("{idf}/tools/cmake/build.cmake")\n'
                    'add_custom_target(__idf_build_target)\n'
                    f'idf_build_set_property(IDF_PATH "{idf}")\n'
                    'idf_build_set_property(__COMPONENT_TARGETS "___idf_main")\n')
                (base/'CMakeLists.txt').write_text(
                    'cmake_minimum_required(VERSION 3.16)\nproject(check NONE)\n'
                    f'set(ENV{{IDF_PATH}} "{fake_idf}")\n'
                    f'set(JRBOT_ENABLE_BABYTALK {enabled})\n'
                    # Stop at project(): run actual root setup, not full SDK/toolchain.
                    'macro(project)\nreturn()\nendmacro()\n'
                    f'include("{firmware}/CMakeLists.txt")\n'
                    f'__build_write_properties("{base}/props.cmake")\n')
                r=subprocess.run(['cmake','-S',str(base),'-B',str(base/'build')],capture_output=True,text=True)
                self.assertEqual(r.returncode,0,r.stderr)
                (base/'components.cmake').write_text(
                    'set(__component____idf_main_COMPONENT_SOURCE project_components)\n'
                    'set(__component____idf_main_COMPONENT_NAME main)\n'
                    'set(__component____idf_main_COMPONENT_ALIAS idf::main)\n'
                    f'set(__component____idf_main_COMPONENT_DIR "{ROOT}/firmware/main")\n')
                r=subprocess.run(['cmake',f'-DBUILD_PROPERTIES_FILE={base}/props.cmake',
                    f'-DCOMPONENT_PROPERTIES_FILE={base}/components.cmake',
                    f'-DCOMPONENT_REQUIRES_FILE={base}/requirements.cmake','-P',
                    str(idf/'tools/cmake/scripts/component_get_requirements.cmake')],capture_output=True,text=True)
                self.assertEqual(r.returncode,0,r.stderr)
                requirements=(base/'requirements.cmake').read_text()
                if enabled=='ON':
                    self.assertIn('stt_engine;sram_pool',requirements)
                else:
                    self.assertNotIn('stt_engine',requirements)
                    self.assertNotIn('sram_pool',requirements)

    @unittest.skipUnless(shutil.which('gcc') and SOURCE.exists(),'host C compiler and official headers required')
    def test_real_capture_and_panel_events_with_cancel_and_read_failure(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder=Path(tmp)
            headers={
                'esp_err.h':'typedef int esp_err_t;\n#define ESP_OK 0\n#define ESP_FAIL -1\n#define ESP_ERR_NO_MEM 2\n#define ESP_ERR_INVALID_STATE 3\n',
                'driver/i2s_std.h':'#include <stddef.h>\n#include "esp_err.h"\ntypedef void* i2s_chan_handle_t;\nint i2s_channel_read(i2s_chan_handle_t,void*,size_t,size_t*,int);',
                'esp_heap_caps.h':'#include <stddef.h>\n#define MALLOC_CAP_SPIRAM 1\n#define MALLOC_CAP_8BIT 2\nvoid *heap_caps_malloc(size_t,int);',
                'esp_timer.h':'#include <stdint.h>\nint64_t esp_timer_get_time(void);',
                'esp_log.h':'#define ESP_LOGI(...) ((void)0)\n',
            }
            for name,content in headers.items():
                path=folder/name;path.parent.mkdir(exist_ok=True);path.write_text('#pragma once\n'+content)
            harness=r'''
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jr_stt.h"
#include "stt_engine.h"
static int reads, calls, text_events,fail_at,cancel_at;
static bool cancel;
int sram_pool_reserve(void){return 0;}
int stt_engine_open(void){return 0;}
void stt_engine_close(void){}
void *heap_caps_malloc(size_t n,int caps){(void)caps;return malloc(n);}
int64_t esp_timer_get_time(void){return reads*16000LL;}
void jr_usb_terminal_emit(const char*line){
 assert(strlen(line)<808);
 if(!strncmp(line,"JR_STT_TEXT:",12)){++text_events;assert(strstr(line,"happy"));}
}
int i2s_channel_read(i2s_chan_handle_t rx,void *out,size_t n,size_t *bytes,int timeout){
 (void)rx;(void)timeout;++reads;
 if(reads==fail_at)return ESP_FAIL;
 if(reads==cancel_at)cancel=true;
 int32_t *raw=out;
 for(size_t j=0;j<n/4;++j)raw[j]=(int32_t)((int)(j%4)-2)*1234*65536;
 *bytes=n;return ESP_OK;
}
int stt_engine_run(const int16_t *pcm,int n,stt_result_t *r){
 ++calls;assert(n==80000 && reads==321);
 for(int i=0;i<n;++i)assert(pcm[i]==(i%4-2)*1234);
 memset(r,0,sizeof(*r));strcpy(r->text,"happy");r->frames=501;return 0;
}
int main(void){
 assert(jr_stt_reserve()==ESP_OK && jr_stt_open()==ESP_OK);
 assert(jr_stt_capture((void*)1,&cancel)==ESP_OK);
 assert(calls==0);jr_stt_transcribe(&cancel);assert(calls==1 && text_events==1);
 jr_stt_transcribe(&cancel);assert(calls==1);
 reads=0;fail_at=12;
 assert(jr_stt_capture((void*)1,&cancel)!=ESP_OK);
 jr_stt_transcribe(&cancel);assert(calls==1);
 reads=0;fail_at=0;cancel_at=20;
 assert(jr_stt_capture((void*)1,&cancel)!=ESP_OK);
 jr_stt_transcribe(&cancel);assert(calls==1);
 jr_stt_close();cancel=false;
 assert(jr_stt_capture((void*)1,&cancel)==ESP_ERR_INVALID_STATE);
}
'''
            (folder/'test.c').write_text(harness)
            result=subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=undefined',
                '-I',str(folder),'-I',str(ROOT/'firmware/module_brain'),'-I',str(ROOT/'firmware/core'),
                '-I',str(SOURCE/'components/stt_engine'),'-I',str(SOURCE/'components/sram_pool'),
                str(ROOT/'firmware/module_brain/jr_stt.c'),str(folder/'test.c'),'-o',str(folder/'test')],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            result=subprocess.run([str(folder/'test')],capture_output=True,text=True,timeout=10)
            self.assertEqual(result.returncode,0,result.stderr)

if __name__=='__main__':unittest.main()
